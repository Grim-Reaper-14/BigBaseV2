#pragma once

namespace big::menu_pages
{
	struct vehicle_catalog_entry final
	{
		const char* display_name;
		const char* model_name;
		const char* category;
	};

	inline constexpr const char* g_vehicle_categories[] =
	{
		"All",
		"Super",
		"Sports",
		"Muscle",
		"SUV & Off-Road",
		"Sedan & Coupe",
		"Motorcycle",
		"Compact & Service"
	};

	inline constexpr vehicle_catalog_entry g_vehicle_catalog[] =
	{
		{"Adder", "adder", "Super"},
		{"Zentorno", "zentorno", "Super"},
		{"T20", "t20", "Super"},
		{"Krieger", "krieger", "Super"},
		{"Emerus", "emerus", "Super"},
		{"Turismo R", "turismor", "Super"},
		{"Osiris", "osiris", "Super"},
		{"Nero", "nero", "Super"},
		{"XA-21", "xa21", "Super"},
		{"Reaper", "reaper", "Super"},
		{"Tempesta", "tempesta", "Super"},
		{"Vagner", "vagner", "Super"},
		{"Entity XF", "entityxf", "Super"},
		{"Entity XXR", "entity2", "Super"},
		{"FMJ", "fmj", "Super"},
		{"Visione", "visione", "Super"},

		{"Banshee", "banshee", "Sports"},
		{"Comet S2", "comet6", "Sports"},
		{"Elegy RH8", "elegy2", "Sports"},
		{"Jester RR", "jester4", "Sports"},
		{"Itali GTO", "italigto", "Sports"},
		{"Pariah", "pariah", "Sports"},
		{"Schlagen GT", "schlagen", "Sports"},
		{"Sultan RS", "sultanrs", "Sports"},
		{"Futo GTX", "futo2", "Sports"},
		{"Calico GTF", "calico", "Sports"},
		{"Growler", "growler", "Sports"},
		{"Cypher", "cypher", "Sports"},

		{"Buffalo STX", "buffalo4", "Muscle"},
		{"Dominator ASP", "dominator7", "Muscle"},
		{"Gauntlet Classic", "gauntlet3", "Muscle"},
		{"Dukes", "dukes", "Muscle"},
		{"Sabre Turbo", "sabregt", "Muscle"},
		{"Vigero ZX", "vigero2", "Muscle"},
		{"Tulip", "tulip", "Muscle"},
		{"Yosemite", "yosemite", "Muscle"},

		{"Baller ST", "baller7", "SUV & Off-Road"},
		{"Granger 3600LX", "granger2", "SUV & Off-Road"},
		{"Rebla GTS", "rebla", "SUV & Off-Road"},
		{"Toros", "toros", "SUV & Off-Road"},
		{"Dubsta 2", "dubsta2", "SUV & Off-Road"},
		{"Sandking XL", "sandking", "SUV & Off-Road"},
		{"Kamacho", "kamacho", "SUV & Off-Road"},
		{"Caracara 4x4", "caracara2", "SUV & Off-Road"},
		{"Draugur", "draugur", "SUV & Off-Road"},
		{"Everon", "everon", "SUV & Off-Road"},
		{"Riata", "riata", "SUV & Off-Road"},
		{"Trophy Truck", "trophytruck", "SUV & Off-Road"},

		{"Tailgater S", "tailgater2", "Sedan & Coupe"},
		{"Schafter V12", "schafter3", "Sedan & Coupe"},
		{"Oracle XS", "oracle2", "Sedan & Coupe"},
		{"Sentinel XS", "sentinel", "Sedan & Coupe"},
		{"Zion Classic", "zion3", "Sedan & Coupe"},
		{"Windsor Drop", "windsor2", "Sedan & Coupe"},
		{"Cognoscenti", "cognoscenti", "Sedan & Coupe"},
		{"Felon GT", "felon2", "Sedan & Coupe"},

		{"Akuma", "akuma", "Motorcycle"},
		{"Bati 801", "bati", "Motorcycle"},
		{"Hakuchou Drag", "hakuchou2", "Motorcycle"},
		{"Shinobi", "shinobi", "Motorcycle"},
		{"Reever", "reever", "Motorcycle"},
		{"Manchez Scout", "manchez2", "Motorcycle"},
		{"Sanchez", "sanchez", "Motorcycle"},
		{"Faggio Sport", "faggio3", "Motorcycle"},

		{"Blista", "blista", "Compact & Service"},
		{"Brioso R/A", "brioso", "Compact & Service"},
		{"Issi Classic", "issi3", "Compact & Service"},
		{"Weevil", "weevil", "Compact & Service"},
		{"Taxi", "taxi", "Compact & Service"},
		{"Bus", "bus", "Compact & Service"},
		{"Journey II", "journey2", "Compact & Service"},
		{"Surfer Custom", "surfer3", "Compact & Service"}
	};
}
