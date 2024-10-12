#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "device_path.hpp"
#include "events.hpp"

using namespace std::string_view_literals;

namespace tcg_parser
{
	enum event_type : uint32_t
	{
		EV_PREBOOT_CERT = 0,
		EV_POST_CODE = 1,
		EV_UNUSED = 2,
		EV_NO_ACTION = 3,
		EV_SEPARATOR = 4,
		EV_ACTION = 5,
		EV_EVENT_TAG = 6,
		EV_S_CRTM_CONTENTS = 7,
		EV_S_CRTM_VERSION = 8,
		EV_CPU_MICROCODE = 9,
		EV_PLATFORM_CONFIG_FLAGS = 10,
		EV_TABLE_OF_DEVICES = 11,
		EV_COMPACT_HASH = 12,
		EV_IPL = 13,
		EV_IPL_PARTITION_DATA = 14,
		EV_NONHOST_CODE = 15,
		EV_NONHOST_CONFIG = 16,
		EV_NONHOST_INFO = 17,
		EV_EFI_VARIABLE = 2147483648,
		EV_EFI_VARIABLE_DRIVER_CONFIG = EV_EFI_VARIABLE + 1,
		EV_EFI_VARIABLE_BOOT = EV_EFI_VARIABLE + 2,
		EV_EFI_BOOT_SERVICES_APPLICATION = EV_EFI_VARIABLE + 3,
		EV_EFI_BOOT_SERVICES_DRIVER = EV_EFI_VARIABLE + 4,
		EV_EFI_RUNTIME_SERVICES_DRIVER = EV_EFI_VARIABLE + 5,
		EV_EFI_GPT_EVENT = EV_EFI_VARIABLE + 6,
		EV_EFI_ACTION = EV_EFI_VARIABLE + 7,
		EV_EFI_PLATFORM_FIRMWARE_BLOB = EV_EFI_VARIABLE + 8,
		EV_EFI_HANDOFF_TABLES = EV_EFI_VARIABLE + 9,
		EV_EFI_HCRTM_EVENT = EV_EFI_VARIABLE + 0x0A,
		EV_EFI_VARIABLE_AUTHORITY = EV_EFI_VARIABLE + 0xE0,
	};

	std::string_view to_string(uint32_t type)
	{
		switch (type)
		{
		case EV_PREBOOT_CERT:
			return "EV_PREBOOT_CERT"sv;
		case EV_POST_CODE:
			return "EV_POST_CODE"sv;
		case EV_UNUSED:
			return "EV_UNUSED"sv;
		case EV_NO_ACTION:
			return "EV_NO_ACTION"sv;
		case EV_SEPARATOR:
			return "EV_SEPARATOR"sv;
		case EV_ACTION:
			return "EV_ACTION"sv;
		case EV_EVENT_TAG:
			return "EV_EVENT_TAG"sv;
		case EV_S_CRTM_CONTENTS:
			return "EV_S_CRTM_CONTENTS"sv;
		case EV_S_CRTM_VERSION:
			return "EV_S_CRTM_VERSION"sv;
		case EV_CPU_MICROCODE:
			return "EV_CPU_MICROCODE"sv;
		case EV_PLATFORM_CONFIG_FLAGS:
			return "EV_PLATFORM_CONFIG_FLAGS"sv;
		case EV_TABLE_OF_DEVICES:
			return "EV_TABLE_OF_DEVICES"sv;
		case EV_COMPACT_HASH:
			return "EV_COMPACT_HASH"sv;
		case EV_IPL:
			return "EV_IPL"sv;
		case EV_IPL_PARTITION_DATA:
			return "EV_IPL_PARTITION_DATA"sv;
		case EV_NONHOST_CODE:
			return "EV_NONHOST_CODE"sv;
		case EV_NONHOST_CONFIG:
			return "EV_NONHOST_CONFIG"sv;
		case EV_NONHOST_INFO:
			return "EV_NONHOST_INFO"sv;
		case EV_EFI_VARIABLE:
			return "EV_EFI_VARIABLE"sv;
		case EV_EFI_VARIABLE_DRIVER_CONFIG:
			return "EV_EFI_VARIABLE_DRIVER_CONFIG"sv;
		case EV_EFI_VARIABLE_BOOT:
			return "EV_EFI_VARIABLE_BOOT"sv;
		case EV_EFI_BOOT_SERVICES_APPLICATION:
			return "EV_EFI_BOOT_SERVICES_APPLICATION"sv;
		case EV_EFI_BOOT_SERVICES_DRIVER:
			return "EV_EFI_BOOT_SERVICES_DRIVER"sv;
		case EV_EFI_RUNTIME_SERVICES_DRIVER:
			return "EV_EFI_RUNTIME_SERVICES_DRIVER"sv;
		case EV_EFI_GPT_EVENT:
			return "EV_EFI_GPT_EVENT"sv;
		case EV_EFI_ACTION:
			return "EV_EFI_ACTION"sv;
		case EV_EFI_PLATFORM_FIRMWARE_BLOB:
			return "EV_EFI_PLATFORM_FIRMWARE_BLOB"sv;
		case EV_EFI_HANDOFF_TABLES:
			return "EV_EFI_HANDOFF_TABLES"sv;
		case EV_EFI_HCRTM_EVENT:
			return "EV_EFI_HCRTM_EVENT"sv;
		case EV_EFI_VARIABLE_AUTHORITY:
			return "EV_EFI_VARIABLE_AUTHORITY"sv;
		}

		return {};
	}

	struct TCG_PCR_EVENT
	{
		uint32_t PCRIndex;
		event_type EventType;
		char Digest[20];
		uint32_t EventSize;
		uint8_t Event[0];
	};

	struct TCG_EfiSpecIdEventAlgorithmSize
	{
		uint16_t HashAlg;
		uint16_t DigestSize;
	};

	struct tcg_pgr_event_2
	{
		uint32_t pcr_index;
		uint32_t event_type;
		std::vector<std::string> digests;
		event_payload_t event;
	};

	struct tcg_pgr_event_1
	{
		uint32_t pcr_index;
		uint32_t event_type;
		std::array<char, 20> digest;
		event_payload_t event;
	};

	event_payload_t read_event_payload(const auto& header, const std::string& buffer)
	{
		using std::size;

		std::istringstream stream(buffer);

		if constexpr (std::same_as<decltype(header), const tcg_pgr_event_1&>)
		{
			if (header.pcr_index != 0)
			{
				return buffer;
			}

			if (header.event_type != EV_NO_ACTION)
			{
				return buffer;
			}

			auto is_not_zero = [](auto c) {
				return c != '\0';
			};

			if (std::ranges::any_of(header.digest, is_not_zero))
			{
				return buffer;
			}

			events::efi_spec_id event;

			if (stream.read(reinterpret_cast<char*>(&event), offsetof(events::efi_spec_id, digest_sizes));
				!stream.good())
			{
				return buffer;
			}

			uint32_t number_of_algorithms;

			if (stream.read(reinterpret_cast<char*>(&number_of_algorithms), sizeof(number_of_algorithms));
				!stream.good())
			{
				return buffer;
			}

			event.digest_sizes.resize(number_of_algorithms);

			if (stream.read(
					reinterpret_cast<char*>(event.digest_sizes.data()),
					number_of_algorithms * sizeof(events::efi_spec_id::digest_size)
				);
				!stream.good())
			{
				return buffer;
			}

			uint8_t vendor_info_size;

			if (stream.read(reinterpret_cast<char*>(&vendor_info_size), sizeof(vendor_info_size)); !stream.good())
			{
				return buffer;
			}

			event.vendor_info.resize(vendor_info_size);

			stream.read(event.vendor_info.data(), vendor_info_size);

			return event;
		}

		switch (header.event_type)
		{
		case EV_EFI_PLATFORM_FIRMWARE_BLOB: {
			events::efi_platform_firmware_blob event;

			if (stream.read(reinterpret_cast<char*>(&event), sizeof(event)); !stream.good())
			{
				return buffer;
			}

			return event;
		}
		case EV_EVENT_TAG:
			return buffer;
		case EV_EFI_BOOT_SERVICES_APPLICATION: {
			events::efi_boot_services_application event;

			if (stream.read(reinterpret_cast<char*>(&event), offsetof(events::efi_boot_services_application, device_path));
				!stream.good())
			{
				return buffer;
			}

			uint64_t size_of_device_path;

			if (stream.read(reinterpret_cast<char*>(&size_of_device_path), sizeof(size_of_device_path)); !stream.good())
			{
				return buffer;
			}

			if (size_of_device_path)
			{
				event.device_path = tcg_parser::device_path::parse(stream);
			}

			return event;
		}
		case EV_EFI_VARIABLE_BOOT: {
			events::efi_variable_boot event;

			if (stream.read(reinterpret_cast<char*>(&event), offsetof(events::efi_variable_boot, unicode_name));
				!stream.good())
			{
				return buffer;
			}

			uint64_t unicode_name_length;

			if (stream.read(reinterpret_cast<char*>(&unicode_name_length), sizeof(unicode_name_length)); !stream.good())
			{
				return buffer;
			}

			uint64_t variable_data_length;

			if (stream.read(reinterpret_cast<char*>(&variable_data_length), sizeof(variable_data_length));
				!stream.good())
			{
				return buffer;
			}

			event.unicode_name.resize(unicode_name_length);

			if (stream.read(reinterpret_cast<char*>(event.unicode_name.data()), unicode_name_length * sizeof(char16_t));
				!stream.good())
			{
				return buffer;
			}

			event.variable_data.resize(variable_data_length);

			if (stream.read(reinterpret_cast<char*>(event.variable_data.data()), variable_data_length); !stream.good())
			{
				return buffer;
			}

			return event;
		}
		}

		return buffer;
	}

	using event_header_t = std::variant<tcg_pgr_event_1, tcg_pgr_event_2>;

	std::optional<tcg_pgr_event_2> read_event_2(
		std::istream& stream,
		const std::vector<events::efi_spec_id::digest_size>& digest_sizes
	)
	{
		using std::size;

		if (!stream.good())
		{
			return {};
		}

		tcg_pgr_event_2 header;

		if (stream.read(reinterpret_cast<char*>(&header.pcr_index), sizeof(header.pcr_index)); !stream.good())
		{
			return {};
		}

		if (stream.read(reinterpret_cast<char*>(&header.event_type), sizeof(header.event_type)); !stream.good())
		{
			return {};
		}

		uint32_t digest_values_count;

		if (stream.read(reinterpret_cast<char*>(&digest_values_count), sizeof(digest_values_count)); !stream.good())
		{
			return {};
		}

		for (auto i = 0u; i < digest_values_count; i++)
		{
			uint16_t hash_alg;

			if (stream.read(reinterpret_cast<char*>(&hash_alg), sizeof(hash_alg)); !stream.good())
			{
				return {};
			}

			auto entry = std::ranges::find_if(digest_sizes, [hash_alg](auto entry) {
				return entry.hash_alg == hash_alg;
			});

			if (entry == end(digest_sizes))
			{
				return {};
			}

			std::string digest(entry->digest_size, '\0');

			if (stream.read(digest.data(), size(digest)); !stream.good())
			{
				return {};
			}

			header.digests.push_back(digest);
		}

		uint32_t event_size;

		if (stream.read(reinterpret_cast<char*>(&event_size), sizeof(event_size)); !stream.good())
		{
			return {};
		}

		std::string buffer(event_size, '\0');

		stream.read(buffer.data(), buffer.size());

		header.event = read_event_payload(header, buffer);

		return header;
	}

	std::optional<tcg_pgr_event_1> read_event_1(std::istream& stream)
	{
		using std::size;

		if (!stream.good())
		{
			return {};
		}

		tcg_pgr_event_1 header;

		if (stream.read(reinterpret_cast<char*>(&header.pcr_index), sizeof(header.pcr_index)); !stream.good())
		{
			return {};
		}

		if (stream.read(reinterpret_cast<char*>(&header.event_type), sizeof(header.event_type)); !stream.good())
		{
			return {};
		}

		if (stream.read(header.digest.data(), size(header.digest)); !stream.good())
		{
			return {};
		}

		uint32_t event_size;

		if (stream.read(reinterpret_cast<char*>(&event_size), sizeof(event_size)); !stream.good())
		{
			return {};
		}

		std::string buffer(event_size, '\0');

		stream.read(buffer.data(), buffer.size());

		header.event = read_event_payload(header, buffer);

		return header;
	}
} // namespace tcg_parser
