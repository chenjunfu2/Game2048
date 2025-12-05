add_rules("mode.debug", "mode.release")

target("Game2048")
	set_kind("binary")
	set_languages("c++20")
	add_files("Game2048/*.cpp")