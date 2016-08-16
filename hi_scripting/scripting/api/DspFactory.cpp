/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


class DspFactory::LibraryLoader : public DynamicObject
{
public:

	LibraryLoader(MainController* mc_):
		mc(mc_)
	{
		handler->setMainController(mc);
		ADD_DYNAMIC_METHOD(load);
        ADD_DYNAMIC_METHOD(list);
	}

	var load(const String &name, const String &password)
	{
		DspFactory *f = dynamic_cast<DspFactory*>(handler->getFactory(name, password));

		return var(f);
	}
    
    var list()
    {
        StringArray s1, s2;
        
        handler->getAllStaticLibraries(s1);
        handler->getAllDynamicLibraries(s2);
        
        String output = "Available static libraries: \n";
        output << s1.joinIntoString("\n");
        
        output << "\nAvailable dynamic libraries: " << "\n";
        output << s2.joinIntoString("\n");
        
        return var(output);
    }


private:

	struct Wrapper
	{
		DYNAMIC_METHOD_WRAPPER_WITH_RETURN(LibraryLoader, load, ARG(0).toString(), ARG(1).toString());
        DYNAMIC_METHOD_WRAPPER_WITH_RETURN(LibraryLoader, list);
	};

	SharedResourcePointer<DspFactory::Handler> handler;

	MainController* mc;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryLoader)
};

struct DspFactory::Wrapper
{
	DYNAMIC_METHOD_WRAPPER_WITH_RETURN(DspFactory, createModule, ARG(0).toString());
	DYNAMIC_METHOD_WRAPPER_WITH_RETURN(DspFactory, getModuleList);
	DYNAMIC_METHOD_WRAPPER_WITH_RETURN(DspFactory, getErrorCode);
};

DspFactory::DspFactory() :
DynamicObject()
{
	ADD_DYNAMIC_METHOD(createModule);
	ADD_DYNAMIC_METHOD(getModuleList);
	ADD_DYNAMIC_METHOD(getErrorCode);
	
}

struct DynamicDspFactory::Wrapper
{
	DYNAMIC_METHOD_WRAPPER_WITH_RETURN(DspFactory, createModule, ARG(0).toString());
	DYNAMIC_METHOD_WRAPPER(DynamicDspFactory, unloadToRecompile);
	DYNAMIC_METHOD_WRAPPER(DynamicDspFactory, reloadAfterRecompile);
};

DynamicDspFactory::DynamicDspFactory(const String &name_, const String& args_) :
name(name_),
args(args_)
{
	openDynamicLibrary();


	ADD_DYNAMIC_METHOD(createModule);
	ADD_DYNAMIC_METHOD(unloadToRecompile);
	ADD_DYNAMIC_METHOD(reloadAfterRecompile);
	
	setProperty("LoadingSuccessful", 0);
	setProperty("Uninitialised", 1);
	setProperty("MissingLibrary", 2);
	setProperty("NoValidLibrary", 3);
	setProperty("NoVersionMatch", 4);
	setProperty("KeyInvalid", 5);
}

void DynamicDspFactory::openDynamicLibrary()
{
#if JUCE_WINDOWS

	const File path = File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Hart Instruments/dll/");

#if JUCE_32BIT
	const String libraryName = name + "_x86.dll";
#else
	const String libraryName = name + "_x64.dll";
#endif

#else
	const File path = File::getSpecialLocation(File::SpecialLocationType::commonApplicationDataDirectory).getChildFile("Application Support/Hart Instruments/lib");
	const String libraryName = name + ".dylib";
#endif

	const String fullLibraryPath = path.getChildFile(libraryName).getFullPathName();
	File f(fullLibraryPath);

	if (!f.existsAsFile())
	{
		errorCode = (int)LoadingErrorCode::MissingLibrary;
	}
	else
	{
		library = new DynamicLibrary();
		library->open(fullLibraryPath);

		errorCode = initialise(args);
	}
}

void GlobalScriptCompileBroadcaster::createDummyLoader()
{
    dummyLibraryLoader = new DspFactory::LibraryLoader(dynamic_cast<MainController*>(this));
}

DspBaseObject * DynamicDspFactory::createDspBaseObject(const String &moduleName) const
{
	if (library != nullptr)
	{
		createDspModule_ c = (createDspModule_)library->getFunction("createDspObject");

		if (c != nullptr)
		{
			return c(moduleName.getCharPointer());
		}
	}

	return nullptr;
}

typedef void(*destroyDspModule_)(DspBaseObject*);

void DynamicDspFactory::destroyDspBaseObject(DspBaseObject *object) const
{
	if (library != nullptr)
	{
		destroyDspModule_ d = (destroyDspModule_)library->getFunction("destroyDspObject");

		if (d != nullptr && object != nullptr)
		{
			d(object);
		}
	}
}

typedef LoadingErrorCode(*init_)(const char* name);

int DynamicDspFactory::initialise(const String &args)
{
	if (library != nullptr)
	{
		init_ d = (init_)library->getFunction("initialise");

		if (d != nullptr)
		{
			isUnloadedForCompilation = false;
			return (int)d(args.getCharPointer());
		}
		else
		{
			return (int)LoadingErrorCode::NoValidLibrary;
		}
	}
	else
	{
		return (int)LoadingErrorCode::MissingLibrary;
	}
}

var DynamicDspFactory::createModule(const String &moduleName) const
{
	if (isUnloadedForCompilation) throw String("Can't load modules for \"unloaded for recompile\" Libraries");

	ScopedPointer<DspInstance> instance = new DspInstance(this, moduleName);

	try
	{
		instance->initialise();
	}
	catch (String errorMessage)
	{
		DBG(errorMessage);
		return var::undefined();
	}

	return var(instance.release());
}

void DynamicDspFactory::unloadToRecompile()
{
	if (isUnloadedForCompilation == false)
	{
		library->close();

		isUnloadedForCompilation = true;
	}
}

void DynamicDspFactory::reloadAfterRecompile()
{
	if (isUnloadedForCompilation == true)
	{
		jassert(library->getNativeHandle() == nullptr);

		isUnloadedForCompilation = false;

		openDynamicLibrary();
	}
}

typedef const Array<Identifier> &(*getModuleList_)();

var DynamicDspFactory::getModuleList() const
{
	if (library != nullptr)
	{
		getModuleList_ d = (getModuleList_)library->getFunction("getModuleList");

		if (d != nullptr)
		{
			const Array<Identifier> *ids = &d();

			Array<var> a;

			for (int i = 0; i < ids->size(); i++)
			{
				a.add(ids->getUnchecked(i).toString());
			}

			return var(a);
		}
		else
		{
			throw String("getModuleList not implemented in Dynamic Library " + name);
		}
	}

	return var::undefined();
}

var DynamicDspFactory::getErrorCode() const
{
	return var(errorCode);
}

void DynamicDspFactory::unload()
{
	library = nullptr;
}

DspBaseObject * StaticDspFactory::createDspBaseObject(const String &moduleName) const
{
	return factory.createFromId(moduleName);
}

void StaticDspFactory::destroyDspBaseObject(DspBaseObject* handle) const
{
	delete handle;
}

var StaticDspFactory::getModuleList() const
{
	Array<var> moduleList;

	for (int i = 0; i < factory.getIdList().size(); i++)
	{
		moduleList.add(factory.getIdList().getUnchecked(i).toString());
	}

	return var(moduleList);
}

var StaticDspFactory::createModule(const String &name) const
{
	ScopedPointer<DspInstance> instance = new DspInstance(this, name);

	try
	{
		instance->initialise();
	}
	catch (String errorMessage)
	{
		DBG(errorMessage);
		return var::undefined();
	}

	return var(instance.release());
}


DspFactory::Handler::Handler()
{
	registerStaticFactories(this);
	tccFactory = new TccDspFactory();
}

DspFactory::Handler::~Handler()
{
	loadedPlugins.clear();
	tccFactory = nullptr;
}

DspInstance * DspFactory::Handler::createDspInstance(const String &factoryName, const String& factoryPassword, const String &moduleName)
{
	return new DspInstance(getFactory(factoryName, factoryPassword), moduleName);
}




DspFactory * DspFactory::Handler::getFactory(const String &name, const String& password)
{
	Identifier id(name);

#if USE_BACKEND
	if (id == tccFactory->getId()) return tccFactory;
#endif

	for (int i = 0; i < staticFactories.size(); i++)
	{
		if (staticFactories[i]->getId() == id)
		{
			return staticFactories[i];
		}
	}

	for (int i = 0; i < loadedPlugins.size(); i++)
	{
		if (loadedPlugins[i]->getId() == id)
		{
			return loadedPlugins[i];
		}
	}

	try
	{
		ScopedPointer<DynamicDspFactory> newLib = new DynamicDspFactory(name, password);
		loadedPlugins.add(newLib.release());
		return loadedPlugins.getLast();
	}
	catch (String errorMessage)
	{
		throw errorMessage;
		return nullptr;
	}
	
	
}



void DspFactory::Handler::setMainController(MainController* mc_)
{
	mc = mc_;
	tccFactory->setMainController(mc);
}