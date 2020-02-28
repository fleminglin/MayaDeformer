#ifndef FLINCOLLIDE_H
#define FLINCOLLIDE_H

#include <maya/MFnTypedAttribute.h>
#include <maya/MDagPath.h>
#include <maya/MBoundingBox.h>
#include <maya/MFnMesh.h>
#include <maya/MPointArray.h>
#include <maya/MPoint.h>
#include <maya/MMatrix.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnNumericData.h>
#include <maya/MItGeometry.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MMeshIntersector.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatMatrix.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MRampAttribute.h>

#include <maya/MPxDeformerNode.h>
#include <maya/MIOStream.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>

#include <stdio.h>
#include <map>
#include <math.h>
#include <tbb/tbb.h>

inline MString MyformatError(const MString &msg, const MString &sourceFile, const int& sourceLine)
{
	MString txt;
	txt += msg;
	txt += "\t File:";
	txt += sourceFile;
	txt += "\t Line:";
	txt += sourceLine;
	return txt;
}

#define myErrCheck(msg)\
	{\
	MString _txt = MyformatError(msg, __FILE__, __LINE__);\
	MGlobal::displayError(_txt);\
	cerr << endl << "Error:" << _txt; \
	}

#define myCheckBool( result )\
	if(!result)\
	{\
	myErrCheck( #result);\
	}

#define myCheckStatus( stat, msg )\
	if(!stat)\
	{\
	myErrCheck(msg); \
	}

#define myCheckObject( obj, msg )\
	if( obj.isNull())\
	{\
	myErrCheck(msg);\
	}

#define myCheckStatusReturn( stat, msg)\
	if(!stat)\
	{\
	myErrCheck( msg );\
	return stat;\
	}


class flinCollide : public MPxDeformerNode
{
public:
						flinCollide();
	virtual				~flinCollide();
	static  void*		creator();
	void				postConstructor();
	MStatus				postConstructor_curveRamp(	MObject& nodeObj,
													MObject& attrObj,
													unsigned int index,
													float position,
													float value,
													unsigned int interpolation
												  );

	virtual MStatus     deform( MDataBlock& data,
								MItGeometry& itGeo,
								const MMatrix& matDeformerL_W,
								unsigned int geomIndex);

	static MStatus		initialize();

	MPointArray			originalPnts;
	MPointArray			finalPnts;
	MFloatVectorArray   deformerNormals;

	MMatrix matColliderL_W;
	MMatrix matColliderW_L;

	MMeshIsectAccelParams accelParam;

	MBoundingBox boxDeformer;
	bool	initialized = false;

public:
	//attributes
	static MTypeId		id;
	static MObject		aColliders;
	static MObject		aMode;
	static MObject		aBulgeShape;
	static MObject		aBulgeAmount;
	static MObject      aRadius;


};

#endif
