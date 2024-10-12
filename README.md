# tcg_parser

A lightweight, header only C++ parser library for TCG event logs.

# Example

```c++
#include <fstream>
#include <iomanip>
#include <iostream>

#include "tcg_parser.hpp"

void handle_event(const tcg_parser::tcg_pgr_event_2& header, auto event)
{
	std::cout << "Unknown event (" << tcg_parser::to_string(header.event_type) << ")" << std::endl;
}

void handle_event(const tcg_parser::tcg_pgr_event_2& header, const tcg_parser::events::efi_boot_services_application& event)
{
	std::cout << "EFI_BOOT_SERVICES_APPLICATION:" << std::endl;

	std::cout << "\tDigests:" << std::endl;

	for (auto& digest : header.digests)
	{
		std::cout << "\t\t- ";

		for (auto c : digest)
		{
			std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<uint8_t>(c));
		}

		std::cout << std::endl;
	}

	std::cout << "\tLocation in memory: 0x" << std::hex << event.image_location_in_memory << std::endl;
	std::cout << "\tLength in memory: 0x" << std::hex << event.image_length_in_memory << std::endl;
	std::cout << "\tLink time address: 0x" << std::hex << event.image_link_time_address << std::endl;

	std::cout << "\tPath: " << tcg_parser::device_path::to_string(event.device_path) << std::endl;
}

void handle_event(const tcg_parser::tcg_pgr_event_2& header, const tcg_parser::events::efi_variable_boot& event)
{
	std::cout << "EFI_VARIABLE_BOOT:" << std::endl;
	std::cout << "\tName: ";

	for (auto c : event.unicode_name)
	{
		std::cout << static_cast<char>(c);
	}

	std::cout << std::endl;
	std::cout << "\tData: ";

	for (auto c : event.variable_data)
	{
		std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
	}

	std::cout << std::endl;
}

int main()
{
	std::ifstream stream("/home/znurre/boot.tcl", std::ios::binary | std::ios::in);

	if (auto header = tcg_parser::read_event_1(stream))
	{
		if (auto spec_event = std::get_if<tcg_parser::events::efi_spec_id>(&header->event))
		{
			if (auto [begin, end] = std::ranges::search(spec_event->signature, "Spec ID Event03"sv); begin == end)
			{
				return 1;
			}

			while (auto header = tcg_parser::read_event_2(stream, spec_event->digest_sizes))
			{
				std::visit(
					[header](auto&& event) {
						handle_event(*header, event);
					},
					header->event
				);
			}
		}
		else
		{
			return 1;
		}
	}

	return 0;
}

```

