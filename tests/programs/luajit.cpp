extern "C" {
#include <luajit-2.1/luajit.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lualib.h>
}
#include <cstdio>

int main() {
	lua_State *L = luaL_newstate();
	if (L == nullptr) {
		std::fprintf(stderr, "Failed to create Lua state\n");
		return 1;
	}

	luaL_openlibs(L);

	const char *script = R"(
print('Hello from LuaJIT!')

-- Create a table as an array
local fruits = {"apple", "banana", "cherry"}

-- Access elements by numerical index (Lua indexes start at 1)
print("First fruit:", fruits[1])

-- Create a table as a dictionary (key-value pairs)
local person = {
    name = "Alice",
    age = 30,
    city = "New York"
}

-- Access elements by key
print("Person's name:", person.name)
print("Person's age:", person["age"])

-- Iterate through a table
for key, value in pairs(person) do
    print(key, value)
end

function fibonacci(n)
	if n < 2 then
		return n
	else
		return fibonacci(n - 1) + fibonacci(n - 2)
	end
end
local result = fibonacci(10)
print('Fibonacci(10) = ' .. result)
)";
	if (luaL_dostring(L, script) != LUA_OK) {
		std::fprintf(stderr, "Error executing Lua script: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1); // Remove error message from stack
		lua_close(L);
		return 1;
	}

	lua_close(L);
	return 0;
}
