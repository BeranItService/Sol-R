/* 
 * Copyright (C) 2014 Cyrille Favreau - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Cyrille Favreau <cyrille_favreau@hotmail.com>
 */

#ifdef WIN32
#include <windows.h>
#else
#include <math.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#endif // WIN32
#include <Logging.h>
#include <iostream>

#include <Consts.h>
#include <io/PDBReader.h>
#include <io/OBJReader.h>
#include <io/FileMarshaller.h>

#include "MoleculeScene.h"

MoleculeScene::MoleculeScene( const std::string& name, const int nbMaxPrimitivePerBox )
: Scene( name, nbMaxPrimitivePerBox )
{
	m_groundHeight = -5000.f;
}

MoleculeScene::~MoleculeScene(void)
{
}

void MoleculeScene::doInitialize()
{
	// initialization
   int   geometryType(rand()%5);
	LOG_INFO(1,"Geometry type: " << geometryType );
	int   atomMaterialType(0);
	float defaultAtomSize(100.f);
	float defaultStickSize(10.f);
	bool loadModels(true);

	std::vector<std::string> proteinNames;
#ifdef WIN32
	// Proteins vector
	HANDLE hFind(nullptr);
	WIN32_FIND_DATA FindData;

	std::string fullFilter("./pdb/*.pdb");
	hFind = FindFirstFile(fullFilter.c_str(), &FindData);
	if( hFind != INVALID_HANDLE_VALUE )
	{
		do
		{
			if( strlen(FindData.cFileName) != 0 )
			{
				std::string shortName(FindData.cFileName);
				shortName = shortName.substr(0,shortName.rfind(".pdb"));
				proteinNames.push_back(shortName);
			}
		}
		while (FindNextFile(hFind, &FindData));
	}
#else
	std::string path="./pdb";
	DIR *dp;
	struct dirent *dirp;
	if((dp  = opendir(path.c_str())) == NULL)
	{
		LOG_ERROR(errno << " opening " << path);
	}
	else
	{
		while ((dirp = readdir(dp)) != NULL)
		{
			std::string filename(dirp->d_name);
			if( filename != "." && filename != ".." &&
				filename.find(".pdb") != std::string::npos && filename.find(".mtl") == std::string::npos )
 			{
				filename = filename.substr(0,filename.find(".pdb"));
				std::string fullPath(path);
				fullPath += "/";
				fullPath += filename;
				proteinNames.push_back(fullPath);
			}
		}
		closedir(dp);
	}
#endif // WIN32

	if( proteinNames.size() != 0 )
	{
		m_currentModel=m_currentModel%proteinNames.size();
		Vertex scale = {200.f,200.f,200.f};
		std::string fileName;

		// Scene
		m_name = proteinNames[m_currentModel];

		// PDB
		PDBReader pdbReader;
#ifdef WIN32
		fileName = "./pdb/";
#endif // WIN32
		fileName += proteinNames[m_currentModel];
		fileName += ".pdb";
		Vertex objectSize = pdbReader.loadAtomsFromFile(
				fileName, *m_gpuKernel,
				static_cast<GeometryType>(geometryType),
				defaultAtomSize, defaultStickSize, atomMaterialType,
				scale, loadModels );

		float size(1.1f);
		objectSize.x *= size;
		objectSize.y *= size;
		objectSize.z *= size;
		if( loadModels )
		{
			fileName = "";
#ifdef WIN32
			fileName += "./pdb/";
#endif // WIN32
			fileName += proteinNames[m_currentModel];
			fileName += ".obj";
			Vertex center={0.f,0.f,0.f};
			OBJReader objReader;
			CPUBoundingBox aabb;
			CPUBoundingBox inAABB;
			objReader.loadModelFromFile(fileName, *m_gpuKernel, center, true, objectSize, true, 1000, false, true, aabb, false, inAABB);
		}
	}
   FileMarshaller fm;
   fm.saveToFile(*m_gpuKernel, "Molecule.irt");
}

void MoleculeScene::doAnimate()
{
   const int nbFrames=120;
   m_rotationAngles.y = static_cast<float>(-2.f*M_PI/nbFrames);
   m_gpuKernel->rotatePrimitives( m_rotationCenter, m_rotationAngles );
	m_gpuKernel->compactBoxes(false);
}

void MoleculeScene::doAddLights()
{
	// Lights 
	m_nbPrimitives = m_gpuKernel->addPrimitive(ptSphere); 
   m_gpuKernel->setPrimitive( m_nbPrimitives,  -5000.f, 5000.f, -15000.f, 1.f, 0.f, 0.f, DEFAULT_LIGHT_MATERIAL ); 
   //m_gpuKernel->setPrimitive( m_nbPrimitives,  0.f, 0.f, 0.f, 1.f, 0.f, 0.f, DEFAULT_LIGHT_MATERIAL ); 
   m_gpuKernel->setPrimitiveIsMovable(m_nbPrimitives,false);
}
