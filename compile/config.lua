local lm = require 'luamake'

local isWindows = lm.os == 'windows'

lm.cxx = 'c++17'

local plat = (function ()
    if isWindows then
        if lm.compiler == "gcc" then
            return "mingw"
        end
        return "msvc"
    end
    return lm.os
end)()
lm.builddir = ("build/%s"):format(plat)

lm.warnings = "error"

if lm.sanitize then
    lm.mode = "debug"
    lm.flags = "-fsanitize=address"
    lm.msvc = {
        defines = "_DISABLE_STRING_ANNOTATION"
    }
end

lm.windows = {
    defines = "_WIN32_WINNT=0x0601",
}
if lm.mode == "debug" and lm.arch == "x86_64" and lm.compiler == "msvc" then
    lm.windows.ldflags = "/STACK:"..0x160000
end

lm.macos = {
    flags = "-Wunguarded-availability",
    sys = "macos10.15",
}

lm.linux = {
    flags = "-fPIC",
}
