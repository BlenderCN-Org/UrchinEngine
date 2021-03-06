#ifndef URCHINENGINE_SINGLETONMANAGER_H
#define URCHINENGINE_SINGLETONMANAGER_H

#include <string>
#include <map>

#include "pattern/singleton/SingletonInterface.h"

#ifdef DWIN
	#ifdef __cplusplus
		extern "C" {
	#endif
	#ifdef DLIB
		#define DLL_EXPORT __declspec(dllexport)
	#else
		#define DLL_EXPORT __declspec(dllimport)
	#endif
#else
	#define DLL_EXPORT
#endif

namespace urchin
{

	/**
	 * Map with the singletons
	 */
	class DLL_EXPORT SingletonManager
	{
		public:
			static void *getSingleton(const std::string &);
			static void addSingleton(const std::string &, SingletonInterface *);
		
			static void destroyAllSingletons();
			
		private:
			SingletonManager() = default;
			~SingletonManager() = default;
		
			static std::map<std::string, SingletonInterface *> singletons;
	};
	
}

#ifdef DWIN
	#ifdef __cplusplus
		}
	#endif
#endif

#endif
