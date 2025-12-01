#pragma once

#include <cstdio>
#include <cassert>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <optional>
//#include <print> //C++23
#include <stdexcept>
#include <termios.h>
#include <unistd.h>
#include <unordered_set>

class Console_Input
{
	public:
		struct Key
		{
			// Just for compability. Actually it's not u16, nor key code.
			char u16KeyCode;
			bool escape = false;
			bool operator==(const Key& rhs) const noexcept {
				return u16KeyCode == rhs.u16KeyCode && escape == rhs.escape;
			}
			bool operator!=(const Key& rhs) const noexcept {
				return u16KeyCode != rhs.u16KeyCode || escape != rhs.escape;
			}
			size_t Hash() const noexcept {
				std::uint16_t hash = u16KeyCode | escape<<8;
				return std::hash<uint16_t>{}(hash);
			}
		};

		struct KeyHash {
			size_t operator()(const Key& obj) const noexcept {
				return obj.Hash();
			}
		};


		using Func = std::function<long(const Key& stKey)>;
	private:
		std::unordered_map<Key, Func, KeyHash> mapRegisterTable;
		termios original;
	public:
	Console_Input(void) {
		termios raw;
		tcgetattr(STDIN_FILENO, &raw);
		original = raw;
		raw.c_lflag &= ~(ECHO | ICANON);
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
	}
	~Console_Input(void) {
		// Restore terminal state.
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
	}

	Console_Input(Console_Input &&) = default;
	Console_Input &operator = (Console_Input &&) = default;

	Console_Input(const Console_Input &) = delete;
	Console_Input &operator = (const Console_Input &) = delete;

	static Key GetTranslateKey(void) {
		Key ret;
		auto ch = std::getchar();
		char tmp;
		switch (ch)
		{
			case 0x1b:
				ret.escape = true;
				tmp = std::getchar();
				assert(tmp == '['); // This is guaranteed to be '['
				ret.u16KeyCode = std::getchar();
				if (ret.u16KeyCode >= '0' && ret.u16KeyCode <= '9') {
					// An extra char for PgDn(6), PgUp(5) and Delete(3)
					tmp = std::getchar();
					assert(tmp == '~');
				}
				break;

			case EOF:
				throw std::runtime_error("Error: EOF encountered in stdin");
			default:
				ret.u16KeyCode = ch;
				ret.escape = false;
				break;
		}
		return ret;
	}

	static void WaitForKey(Key target) {
		Key get;
		do {
			get = GetTranslateKey();
		} while (get != target);
		return;
	}

	static Key WaitForKeys(const std::unordered_set<Key, KeyHash> &targets) {
		Key get;
		do {
			get = GetTranslateKey();
		} while (!targets.contains(get));
		return get;
	}

	std::optional<long> Once(void) const {
		Key get = GetTranslateKey();
		auto it = mapRegisterTable.find(get);
		if (it == mapRegisterTable.end()) {
			return {};
		}

		return it->second(it->first);
	}

	long AtLeastOne(void) const {
		std::optional<long> ret;
		do {
			ret = Once();
		} while (!ret.has_value());
		return ret.value();
	}

	static Key WaitAnyKey(void) noexcept {
		return GetTranslateKey();
	}

	void RegisterKey(const Key& key, Func callback) {
		mapRegisterTable[key] = callback;
	}

};
