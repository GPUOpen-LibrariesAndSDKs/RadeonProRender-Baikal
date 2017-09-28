#include "clw_class.h"

namespace Baikal
{
#if defined(_WIN32) || defined (WIN32)
   std::vector<ClwClass*> ClwClass::m_clw_classes;
#endif

   void ClwClass::Update()
   {
#if defined(_WIN32) || defined (WIN32)
       for (auto it = m_clw_classes.begin(); it != m_clw_classes.end(); it++)
       {
           ClwClass* clw_class = (*it);

           clw_class->Rebuild(clw_class->GetBuildOpts());
       }
#endif
   }
}