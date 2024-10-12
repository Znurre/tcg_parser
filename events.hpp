#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "device_path.hpp"

namespace tcg_parser
{
#pragma pack(push, 1)

	namespace events
	{
		struct efi_spec_id
		{
			struct digest_size
			{
				uint16_t hash_alg;
				uint16_t digest_size;
			};

			std::array<char, 16> signature;
			uint32_t platform_class;
			uint8_t spec_version_minor;
			uint8_t spec_version_major;
			uint8_t spec_errata;
			uint8_t uint_n_size;
			std::vector<digest_size> digest_sizes;
			std::string vendor_info;
		};

		struct efi_boot_services_application
		{
			uint64_t image_location_in_memory;
			uint64_t image_length_in_memory;
			uint64_t image_link_time_address;
			std::vector<device_path_t> device_path;
		};

		struct efi_variable_boot
		{
			std::array<uint8_t, 16> variable_name;
			std::u16string unicode_name;
			std::vector<uint8_t> variable_data;
		};

		struct efi_platform_firmware_blob
		{
			uint64_t blob_base;
			uint64_t blob_length;
		};

		struct s_crtm_version
		{
		};

		using raw_event_t = std::string;
	} // namespace events

#pragma pack(pop)

	using event_payload_t = std::variant<
		events::raw_event_t,
		events::efi_spec_id,
		events::efi_boot_services_application,
		events::efi_variable_boot,
		events::efi_platform_firmware_blob>;
} // namespace tcg_parser
