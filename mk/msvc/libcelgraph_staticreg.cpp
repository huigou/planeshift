// This file is automatically generated.
#include "cssysdef.h"
#include "csutil/scf.h"

// Put static linking stuff into own section.
// The idea is that this allows the section to be swapped out but not
// swapped in again b/c something else in it was needed.
#if !defined(CS_DEBUG) && defined(CS_COMPILER_MSVC)
#pragma const_seg(".CSmetai")
#pragma comment(linker, "/section:.CSmetai,r")
#pragma code_seg(".CSmeta")
#pragma comment(linker, "/section:.CSmeta,er")
#pragma comment(linker, "/merge:.CSmetai=.CSmeta")
#endif

namespace csStaticPluginInit
{
static char const metainfo_celgraph[] =
"<?xml version=\"1.0\"?>"
"<!-- celgraph.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>cel.celpath</name>"
"        <implementation>celPath</implementation>"
"	<description>CEL Path</description>"
"      </class>"
"      <class>"
"        <name>cel.celgraph</name>"
"        <implementation>celGraph</implementation>"
"	<description>CEL Graph</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef celPath_FACTORY_REGISTER_DEFINED 
  #define celPath_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(celPath) 
  #endif
  #ifndef celGraph_FACTORY_REGISTER_DEFINED 
  #define celGraph_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(celGraph) 
  #endif

class celgraph
{
SCF_REGISTER_STATIC_LIBRARY(celgraph,metainfo_celgraph)
  #ifndef celPath_FACTORY_REGISTERED 
  #define celPath_FACTORY_REGISTERED 
    celPath_StaticInit celPath_static_init__; 
  #endif
  #ifndef celGraph_FACTORY_REGISTERED 
  #define celGraph_FACTORY_REGISTERED 
    celGraph_StaticInit celGraph_static_init__; 
  #endif
public:
 celgraph();
};
celgraph::celgraph() {}

}
