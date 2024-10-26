#pragma once

#include <array>
#include <cstdint>
#include <format>
#include <iostream>
#include <istream>
#include <numeric>
#include <string>
#include <variant>
#include <vector>

#include "acpi.hpp"

namespace tcg_parser
{
#pragma pack(push, 1)

	namespace device_path
	{
		struct unknown
		{
			uint8_t type;
			uint8_t sub_type;
			uint16_t length;
		};

		namespace hardware
		{
			struct pci
			{
				uint8_t function;
				uint8_t device;
			};

			struct mmio
			{
				uint32_t memory_type;
				uint64_t start_address;
				uint64_t end_address;
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
				std::array<uint8_t, 8> extended_unique_identifier;
			};

			struct sata
			{
				uint16_t hba_port;
				uint16_t port_multiplier_port;
				uint16_t logical_unit_number;
			};

			struct lun
			{
				uint8_t lun;
			};

			struct usb
			{
				uint8_t parent_port;
				uint8_t interface;
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

			struct relative_offset_range
			{
				uint32_t reserved;
				uint64_t starting_offset;
				uint64_t ending_offset;
			};
		} // namespace media
	}	  // namespace device_path

#pragma pack(pop)

	using device_path_t = std::variant<
		device_path::unknown,
		device_path::hardware::pci,
		device_path::hardware::mmio,
		device_path::acpi::acpi,
		device_path::acpi::extended_acpi,
		device_path::messaging::nvme_namespace,
		device_path::messaging::sata,
		device_path::messaging::lun,
		device_path::messaging::usb,
		device_path::media::hard_drive,
		device_path::media::file,
		device_path::media::piwg_firmware_volume,
		device_path::media::piwg_firmware_files,
		device_path::media::relative_offset_range>;

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
			unknown header;

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
					case 0x1: {
						hardware::pci path;

						if (stream.read(reinterpret_cast<char*>(&path), sizeof(path)); !stream.good())
						{
							return paths;
						}

						paths.push_back(path);

						continue;
					}
					case 0x3: { // Memory Mapped
						hardware::mmio path;

						if (stream.read(reinterpret_cast<char*>(&path), sizeof(path)); !stream.good())
						{
							return paths;
						}

						paths.push_back(path);

						continue;
					}
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
					case 0x5: { // USB
						messaging::usb path;

						if (stream.read(reinterpret_cast<char*>(&path), sizeof(path)); !stream.good())
						{
							return paths;
						}

						paths.push_back(path);

						continue;
					}
					case 0x11: { // LUN
						messaging::lun path;

						if (stream.read(reinterpret_cast<char*>(&path), sizeof(path)); !stream.good())
						{
							return paths;
						}

						paths.push_back(path);

						continue;
					}
					case 0x12: { // SATA
						messaging::sata path;

						if (stream.read(reinterpret_cast<char*>(&path), sizeof(path)); !stream.good())
						{
							return paths;
						}

						paths.push_back(path);

						continue;
					}
					case 0x17: { // NVM Express Namespace
						messaging::nvme_namespace path;

						if (stream.read(reinterpret_cast<char*>(&path), sizeof(path)); !stream.good())
						{
							return paths;
						}

						paths.push_back(path);

						continue;
					}
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
					case 0x8: { // Relative offset range
						media::relative_offset_range path;

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

				paths.push_back(header);

				if (stream.seekg(header.length - sizeof(header), std::ios::cur); !stream.good())
				{
					return paths;
				}
			}

			return paths;
		}

		std::string to_string(const device_path::unknown& path)
		{
			return std::format("\\Unknown({:x}, {:x})", path.type, path.sub_type);
		}

		std::string to_string(const device_path::hardware::pci& path)
		{
			return std::format("\\Pci(0x{:x}, 0x{:x})", path.device, path.function);
		}

		std::string to_string(const device_path::hardware::mmio& path)
		{
			return std::format("\\MemoryMapped({}, 0x{:x}, 0x{:x})", path.memory_type, path.start_address, path.end_address);
		}

		std::string to_string(const device_path::acpi::acpi& path)
		{
			switch (path.hid)
			{
			case EFIDP_ACPI_PCI_ROOT_HID:
				return std::format("\\PciRoot(0x{:x})", path.uid);
			case EFIDP_ACPI_CONTAINER_0A05_HID:
			case EFIDP_ACPI_CONTAINER_0A06_HID:
				return "\\AcpiContainer()";
			case EFIDP_ACPI_PCIE_ROOT_HID:
				return std::format("\\PcieRoot(0x{:x})", path.uid);
			case EFIDP_ACPI_EC_HID:
				return "\\EmbeddedController()";
			case EFIDP_ACPI_FLOPPY_HID:
				return std::format("\\Floppy(0x{:x})", path.uid);
			case EFIDP_ACPI_KEYBOARD_HID:
				return std::format("\\Keyboard(0x{:x})", path.uid);
			case EFIDP_ACPI_SERIAL_HID:
				return std::format("\\Serial(0x{:x})", path.uid);
			default:
				return std::format("\\Acpi(0x{:8x},0x{:x})", path.hid, path.uid);
			}
		}

		std::string to_string(const device_path::acpi::extended_acpi& path)
		{
			return "\\AcpiExp()";
		}

		std::string to_string(const device_path::messaging::nvme_namespace& path)
		{
			return std::format(
				"\\NVMe(0x{:x}, {:02X}-{:02X}-{:02X}-{:02X}-{:02X}-{:02X}-{:02X}-{:02X})",
				path.namespace_identifier,
				path.extended_unique_identifier[0],
				path.extended_unique_identifier[1],
				path.extended_unique_identifier[2],
				path.extended_unique_identifier[3],
				path.extended_unique_identifier[4],
				path.extended_unique_identifier[5],
				path.extended_unique_identifier[6],
				path.extended_unique_identifier[7]
			);
		}

		std::string to_string(const device_path::messaging::sata& path)
		{
			return std::format("\\Sata({}, {}, {})", path.hba_port, path.port_multiplier_port, path.logical_unit_number);
		}

		std::string to_string(const device_path::messaging::lun& path)
		{
			return std::format("\\Unit({})", path.lun);
		}

		std::string to_string(const device_path::messaging::usb& path)
		{
			return std::format("\\USB({}, {})", path.parent_port, path.interface);
		}

		std::string to_string(const media::file& path)
		{
			std::string target;

			std::transform(begin(path.path), end(path.path), back_inserter(target), [](auto character) {
				return static_cast<char>(character);
			});

			return target;
		}

		std::string to_string(const media::piwg_firmware_volume& path)
		{
			return std::format(
				"\\FvVol({{{:02X}{:02X}{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:"
				"02X}{:02X}}})",
				path.firmware_volume_name[0],
				path.firmware_volume_name[1],
				path.firmware_volume_name[2],
				path.firmware_volume_name[3],
				path.firmware_volume_name[4],
				path.firmware_volume_name[5],
				path.firmware_volume_name[6],
				path.firmware_volume_name[7],
				path.firmware_volume_name[8],
				path.firmware_volume_name[9],
				path.firmware_volume_name[10],
				path.firmware_volume_name[11],
				path.firmware_volume_name[12],
				path.firmware_volume_name[13],
				path.firmware_volume_name[14],
				path.firmware_volume_name[15]
			);
		}

		std::string to_string(const media::piwg_firmware_files& path)
		{
			return std::format(
				"\\FvFile({{{:02X}{:02X}{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:"
				"02X}{:02X}}})",
				path.firmware_file_name[0],
				path.firmware_file_name[1],
				path.firmware_file_name[2],
				path.firmware_file_name[3],
				path.firmware_file_name[4],
				path.firmware_file_name[5],
				path.firmware_file_name[6],
				path.firmware_file_name[7],
				path.firmware_file_name[8],
				path.firmware_file_name[9],
				path.firmware_file_name[10],
				path.firmware_file_name[11],
				path.firmware_file_name[12],
				path.firmware_file_name[13],
				path.firmware_file_name[14],
				path.firmware_file_name[15]
			);
		}

		std::string to_string(const media::hard_drive& path)
		{
			switch (path.signature_type)
			{
			case 1: // MBR
				return std::format(
					"\\HD({},MBR,0x{:x},0x{:x},0x{:x})",
					path.partition_number,
					*reinterpret_cast<const uint32_t*>(path.signature.data()),
					path.partition_start,
					path.partition_size
				);
			case 2: // GPT
				return std::format(
					"\\HD({},GPT,{{{:02X}{:02X}{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}-{:"
					"02X}{:02X}{:02X}{:02X}{:02X}{:02X}}},0x{:"
					"x},0x{:x})",
					path.partition_number,
					path.signature[0],
					path.signature[1],
					path.signature[2],
					path.signature[3],
					path.signature[4],
					path.signature[5],
					path.signature[6],
					path.signature[7],
					path.signature[8],
					path.signature[9],
					path.signature[10],
					path.signature[11],
					path.signature[12],
					path.signature[13],
					path.signature[14],
					path.signature[15],
					path.partition_start,
					path.partition_size
				);
			default:
				return std::format(
					"\\HD({},{},{:x},{:x}",
					path.partition_number,
					path.signature_type,
					path.partition_start,
					path.partition_size
				);
			}
		}

		std::string to_string(const media::relative_offset_range& path)
		{
			return std::format("\\Offset(0x{:x}, 0x{:x})", path.starting_offset, path.ending_offset);
		}

		std::string to_string(const device_path_t& path)
		{
			return std::visit(
				[](auto&& path) {
					return to_string(path);
				},
				path
			);
		}

		std::string to_string(const std::vector<device_path_t>& paths)
		{
			return std::accumulate(begin(paths), end(paths), std::string(), [](auto string, auto path) {
				return string + to_string(path);
			});
		}
	} // namespace device_path
} // namespace tcg_parser
