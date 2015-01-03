function ActionUsesGCC()
	return ("gmake" == _ACTION or "codelite" == _ACTION or "codeblocks" == _ACTION or (_ACTION and _ACTION:find("xcode")))
end

function ActionUsesMSVC()
	return (_ACTION and _ACTION:find("vs"))
end

function EnableOpenMP()
     if ActionUsesGCC() then
        buildoptions ( "-fopenmp" )
	if kindVal ~= "StaticLib" then
	  links		 ( { "gomp", "pthread" } )
	end
	linkoptions	 ( "-fopenmp" )
      end
      if ActionUsesMSVC() then
        buildoptions ( "/openmp" )
      end
end

newoption
{
	trigger		= "openmp",
	description	= "Enable OpenMP support which will add the OPENMP_ENABLED define."
}

-- A solution contains projects, and defines the available configurations
solution "CppReflector"
   configurations { "Debug", "Release" }
 
   -- A project defines one build target
   project "CppReflector"
      kind "ConsoleApp"
      language "C++"
      files { "**.h", "**.cpp", "**.xh"}
      flags { "Cpp11" }

      if _OPTIONS['openmp'] ~= nil then EnableOpenMP() end

      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }

