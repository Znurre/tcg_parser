#pragma once

#include <array>
#include <cstdint>
#include <istream>
#include <string>
#include <variant>
#include <vector>

namespace tcg_parser
{
#pragma pack(push, 1)

	namespace device_path
	{
		namespace hardware
		{
			struct pci
			{
				uint8_t function;
				uint8_t device;
			};
		} // namespace hardware

		namespace acpi
		{
			struct acpi
			{
				uint32_t hid;
				uint32_t uid;
			};

			struct extended_acpi
			{
				std::variant<uint32_t, std::u16string> hid;
				std::variant<uint32_t, std::u16string> uid;
				std::variant<uint32_t, std::u16string> cid;
			};
		} // namespace acpi

		namespace messaging
		{
			struct nvme_namespace
			{
				uint32_t namespace_identifier;
				uint64_t extended_unique_identifier;
			};
		} // namespace messaging

		namespace media
		{
			struct hard_drive
			{
				uint32_t partition_number;
				uint64_t partition_start;
				uint64_t partition_size;
				std::array<uint8_t, 16> signature;
				uint8_t partition_format;
				uint8_t signature_type;
			};

			struct file
			{
				std::u16string path;
			};

			struct piwg_firmware_volume
			{
				std::array<uint8_t, 16> firmware_volume_name;
			};

			struct piwg_firmware_files
			{
				std::array<uint8_t, 16> firmware_file_name;
			};
		} // namespace media
	}	  // namespace device_path

#pragma pack(pop)

	using device_path_t = std::variant<
		device_path::hardware::pci,
		device_path::acpi::acpi,
		device_path::acpi::extended_acpi,
		device_path::messaging::nvme_namespace,
		device_path::media::hard_drive,
		device_path::media::file,
		device_path::media::piwg_firmware_volume,
		device_path::media::piwg_firmware_files>;

	namespace device_path
	{
		namespace details
		{
			std::u16string read_string(std::istream& stream)
			{
				std::u16string storage;

				while (stream.good())
				{
					char16_t character;

					if (stream.read(reinterpret_cast<char*>(&character), sizeof(character)); !character)
					{
						break;
					}

					storage += character;
				}

				return storage;
			}
		} // namespace details

		std::vector<device_path_t> parse(std::istream& stream)
		{
#pragma pack(push, 1)

			struct
			{
				uint8_t type;
				uint8_t sub_type;
				uint16_t length;
			} header;

#pragma pack(pop)

			std::vector<device_path_t> paths;

			while (stream.good())
			{
				if (stream.read(reinterpret_cast<char*>(&header), sizeof(header)); !stream.good())
				{
					return paths;
				}

				switch (header.type)
				{
				case 0x1: // Hardware device path
					switch (header.sub_type)
					{
					case 0x1:
						hardware::pci path;

						if (stream.read(reinterpret_cast<char*>(&path), sizeof(path)); !stream.good())
						{
							return paths;
						}

						paths.push_back(path);

						continue;
					}

					break;
				case 0x2: // ACPI device path
					switch (header.sub_type)
					{
					case 0x1: // ACPI device path
						acpi::acpi path;

						if (stream.read(reinterpret_cast<char*>(&path), sizeof(path)); !stream.good())
						{
							return paths;
						}

						paths.push_back(path);

						continue;
					case 0x2: // Expanded ACPI device path
					{
#pragma pack(push, 1)

						struct
						{
							uint32_t hid;
							uint32_t uid;
							uint32_t cid;
						} block;

#pragma pack(pop)

						if (stream.read(reinterpret_cast<char*>(&block), sizeof(block)); !stream.good())
						{
							return paths;
						}

						acpi::extended_acpi path = {
							.hid = block.hid,
							.uid = block.uid,
							.cid = block.cid,
						};

						if (auto hidstr = details::read_string(stream); !empty(hidstr))
						{
							path.hid = hidstr;
						}

						if (auto uidstr = details::read_string(stream); !empty(uidstr))
						{
							path.uid = uidstr;
						}

						if (auto cidstr = details::read_string(stream); !empty(cidstr))
						{
							path.cid = cidstr;
						}

						paths.push_back(path);

						continue;
					}
					case 0x3: // _ADR device path
						break;
					case 0x4: // NVDIMM device
						break;
					}

					break;
				case 0x3: // Messaging device path
					switch (header.sub_type)
					{
					case 0x17: // NVM Express Namespace
						messaging::nvme_namespace path;

						if (stream.read(reinterpret_cast<char*>(&path), sizeof(path)); !stream.good())
						{
							return paths;
						}

						paths.push_back(path);

						continue;
					}

					break;
				case 0x4: // Media device path
					switch (header.sub_type)
					{
					case 0x1: // Hard drive
						media::hard_drive path;

						if (stream.read(reinterpret_cast<char*>(&path), sizeof(path)); !stream.good())
						{
							return paths;
						}

						paths.push_back(path);

						continue;
					case 0x2: // CD-ROM
						break;
					case 0x3: // Vendor
						break;
					case 0x4: { // File path
						media::file path {
							.path = details::read_string(stream),
						};

						if (!stream.good())
						{
							return paths;
						}

						paths.push_back(path);

						continue;
					}
					case 0x5: // Media protocol
						break;
					case 0x6: { // PIWG firmware files
						media::piwg_firmware_files path;

						if (stream.read(reinterpret_cast<char*>(&path), sizeof(path)); !stream.good())
						{
							return paths;
						}

						paths.push_back(path);

						continue;
					}
					case 0x7: { // PIWG firmware volume
						media::piwg_firmware_volume path;

						if (stream.read(reinterpret_cast<char*>(&path), sizeof(path)); !stream.good())
						{
							return paths;
						}

						paths.push_back(path);

						continue;
					}
					}
					break;
				case 0x5: // BIOS boot specification device path
					break;
				case 0x7f: // End of hardware device path
					if (header.sub_type == 0xFF)
					{
						return paths;
					}
					break;
				}

				if (stream.seekg(header.length - sizeof(header), std::ios::cur); !stream.good())
				{
					return paths;
				}
			}

			return paths;
		}
	} // namespace device_path
} // namespace tcg_parser
