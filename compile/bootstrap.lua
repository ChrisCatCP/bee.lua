local lm = require "luamake"

lm:executable "bootstrap" {
    bindir = "$bin",
    deps = {
        "source_bee",
        "source_lua",
    },
    includes = { "3rd/lua", "." },
    sources = "bootstrap/main.cpp",
    windows = {
        deps = "bee_utf8_crt",
        sources = "bootstrap/bootstrap.rc",
    },
    macos = {
        defines = "LUA_USE_MACOSX",
        links = { "m", "dl" },
    },
    linux = {
        defines = "LUA_USE_LINUX",
        links = { "m", "dl" }
    },
    netbsd = {
        defines = "LUA_USE_LINUX",
        links = "m",
    },
    freebsd = {
        defines = "LUA_USE_LINUX",
        links = "m",
    },
    openbsd = {
        defines = "LUA_USE_LINUX",
        links = "m",
    },
    android = {
        defines = "LUA_USE_LINUX",
        links = { "m", "dl" }
    },
    msvc = {
        ldflags = "/IMPLIB:$obj/bootstrap.lib"
    },
    mingw = {
        ldflags = "-Wl,--out-implib,$obj/bootstrap.lib"
    },
}

lm:copy "copy_script" {
    inputs = "bootstrap/main.lua",
    outputs = "$bin/main.lua",
    deps = "bootstrap",
}

if not lm.notest then
    local exe = lm.os == "windows" and ".exe" or ""
    local tests = {}
    local fs = require "bee.filesystem"
    local rootdir = fs.path(lm.workdir)
    for file in fs.pairs(rootdir / "test", "r") do
        if file:extension() == ".lua" then
            tests[#tests+1] = fs.relative(file, rootdir):lexically_normal():string()
        end
    end
    table.sort(tests)

    lm:rule "test" {
        args = { "$bin/bootstrap"..exe, "@test/test.lua", "--touch", "$out" },
        description = "Run test.",
        pool = "console",
    }
    lm:build "test" {
        rule = "test",
        deps = { "bootstrap", "copy_script" },
        inputs = tests,
        outputs = "$obj/test.stamp",
    }
end
