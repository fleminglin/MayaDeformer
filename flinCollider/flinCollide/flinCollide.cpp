#include "flinCollide.h"

MTypeId flinCollide::id(0x00000238);
MObject flinCollide::aColliders;
MObject flinCollide::aBulgeShape;
MObject	flinCollide::aBulgeAmount;
MObject flinCollide::aMode;
MObject flinCollide::aRadius;


flinCollide::flinCollide()
{
}

flinCollide::~flinCollide()
{
}

void* flinCollide::creator()
{
	return new flinCollide();
}

void flinCollide::postConstructor()
{
	//create entries for rampAttribute;
	MObject thisNode = this->thisMObject();
	
	//jump to helper function so we can check the status
	postConstructor_curveRamp(thisNode, aBulgeShape, 0, 0.0f, 0.5f, 2);
	postConstructor_curveRamp(thisNode, aBulgeShape, 1, 0.15f, 1.0f, 2);
	postConstructor_curveRamp(thisNode, aBulgeShape, 2, 1.0f, 0.0f, 2);
}

MStatus flinCollide::postConstructor_curveRamp( MObject& nodeObj, 
												MObject& attrObj, 
												unsigned int index, 
												float position, 
												float value, 
												unsigned int interpolation)
{
	MStatus status;
	MPlug plugRamp(nodeObj, attrObj);
	MPlug plugElements = plugRamp.elementByLogicalIndex(index, &status);
	myCheckStatusReturn(status, "plugRamp.elementByLogicalIndex");
	MPlug plugPostion = plugElements.child(0, &status);
	myCheckStatusReturn(status, "plugElements.child 0");
	plugPostion.setFloat(position);

	MPlug plugValue = plugElements.child(1, &status);
	myCheckStatusReturn(status, "plugElements.child 1");
	plugValue.setFloat(value);

	MPlug plugInterpolation = plugElements.child(2, &status);
	myCheckStatusReturn(status, "plugElements.child 1");
	plugInterpolation.setInt(interpolation);

	return status;
}

MStatus flinCollide::initialize()
{
	MStatus status;
	MFnNumericAttribute nAttr;
	MFnTypedAttribute tAttr;
	MFnMatrixAttribute mAttr;
	MFnEnumAttribute  eAttr;
	MRampAttribute	  rAttr;

	aColliders = tAttr.create("collideGeo", "collideGeo", MFnData::kMesh, &status);
	myCheckStatusReturn(status, "can't initialize collideGeo");
	tAttr.setArray(true);
	tAttr.setDisconnectBehavior(MFnAttribute::kDelete);
	addAttribute(aColliders);
	attributeAffects(aColliders, outputGeom);

	aMode = eAttr.create("mode", "mode", 0, &status);
	myCheckStatusReturn(status, "eAttr.create(mode, mode)");
	eAttr.addField("relax", 0);
	eAttr.addField("trace", 1);
	eAttr.setKeyable(true);
	addAttribute(aMode);
	attributeAffects(aMode, outputGeom);

	aRadius = nAttr.create("radius", "radius", MFnNumericData::kFloat, 2.0);
	nAttr.setKeyable(true);
	nAttr.setMin(0);
	nAttr.setSoftMax(10);
	addAttribute(aRadius);
	attributeAffects(aRadius, outputGeom);

	aBulgeAmount = nAttr.create("bulgeAmount", "bulgeAmount", MFnNumericData::kFloat, 1.0);
	nAttr.setKeyable(true);
	nAttr.setSoftMin(0);
	nAttr.setSoftMax(20);
	addAttribute(aBulgeAmount);
	attributeAffects(aBulgeAmount, outputGeom);

	aBulgeShape = rAttr.createCurveRamp("bulgeShape", "bulgeShape", &status);
	myCheckStatusReturn(status, "rAttr.createCurveRamp");
	addAttribute(aBulgeShape);
	attributeAffects(aBulgeShape, outputGeom);

	MGlobal::executeCommand("makePaintable -attrType multiFloat -sm deformer flinCollide weights;");

	return MS::kSuccess;
}

MStatus flinCollide::deform(MDataBlock& data, MItGeometry& iterGeo, const MMatrix& matDeformerL_W, unsigned int geoIndex)
{
	MStatus		status;
	MObject		thisNode = this->thisMObject();

	//attributes
	MArrayDataHandle hColliders = data.inputArrayValue(aColliders, &status);
	myCheckStatusReturn(status, "fail to get collider array");
	int			modeValue = data.inputValue(aMode).asInt();
	float		env = data.inputValue(envelope).asFloat();
	float       radius = data.inputValue(aRadius).asFloat();
	float       bulgeVal = data.inputValue(aBulgeAmount).asFloat();
	MRampAttribute  bulgeShape = MRampAttribute(thisNode, aBulgeShape, &status);
	myCheckStatusReturn(status, "fail to get ramp Attribute in deform");

	//get deformer local to world Matrix
	MMatrix matDeformerW_L = matDeformerL_W.inverse();

	//get the deformer mesh
	MArrayDataHandle hInput = data.outputArrayValue(input, &status);
	myCheckStatus(status, "MArrayDataHandle hInput");
	hInput.jumpToArrayElement(geoIndex);
	MDataHandle	hInputGeom = hInput.outputValue().child(inputGeom);
	MObject		oDeformer = hInputGeom.asMesh();

	//get the normals, original pnts, finalPnts, total vertexs from deformer
	MFnMesh fnDeformer(oDeformer, &status);
	fnDeformer.getVertexNormals(true, deformerNormals);
	iterGeo.allPositions(originalPnts);
	//reset the deformer if
	//		1) during initialization   2) deformer membership change 3) mode set to relax
	if (finalPnts.length() == 0 || finalPnts.length() != iterGeo.count() || modeValue == 0)
	{
		iterGeo.allPositions(finalPnts);
	}
	unsigned vtxCount = (unsigned)iterGeo.count();

	//if env = 0, end;
	if (env == 0)
		return MS::kSuccess;
	
	// -------------------------------------------------------------
	// create deformer bounding box if not initialized
	// -------------------------------------------------------------
	if (initialized == false)
	{
		for (unsigned int i = 0; i < finalPnts.length(); i++)
		{
			MFloatPoint deformerWorldPnts = finalPnts[i] * matDeformerL_W;
			boxDeformer.expand(deformerWorldPnts);
		}

		initialized = true;
	}

	//if not collider is connected, end;

	if (hColliders.elementCount() == 0)
		return MS::kSuccess;

	//check for valid collider connections
	MIntArray colliderArray;
	for (unsigned int i = 0; i < hColliders.elementCount(); i++)
	{
		status = hColliders.jumpToElement(i);
		myCheckStatusReturn(status, "faile hColliders.jumpToElement(i) ");
		unsigned int index = hColliders.elementIndex();

		MPlug pColliders(thisNode, aColliders);
		MPlug pCollider = pColliders.elementByLogicalIndex(index, &status);
		myCheckStatusReturn(status, "fail to get pColliders.elementByLogicalIndex");
		if (pCollider.isConnected() == true)
		{
			colliderArray.append(index);
		}
	}



	// -----------------------------------------------------------------
	// collider base deformation
	// -----------------------------------------------------------------

	MDataHandle hCollider;
	MMeshIntersector	intersector;
	for (unsigned int i = 0; i < colliderArray.length(); i++)
	{
		bool hasCollision = false;
		double maxDisplacement = 0;

		// -------------------------------------------------------------
		// get collider datahandle
		// -------------------------------------------------------------

		unsigned int colIndex = colliderArray[i];
		hColliders.jumpToElement(colIndex);
		hCollider = hColliders.inputValue();

		// -------------------------------------------------------------
		// get collider world Mesh and local Mesh
		// -------------------------------------------------------------

		MObject		oColliderTrans = hCollider.asMeshTransformed();
		MObject		oCollider = hCollider.asMesh();

		// -------------------------------------------------------------
		// create fnMesh for collider
		// get the colliderPnts from collider
		// -------------------------------------------------------------

		MFnMesh fnColliderTrans(oColliderTrans, &status);
		MFnMesh fnCollider(oCollider, &status);

		MPointArray			colliderPnts;
		status = fnColliderTrans.getPoints(colliderPnts);
		myCheckStatus(status, "fnColliderTrans.getPoints(colliderPnts)");

		// -------------------------------------------------------------
		// get collider transform matrix
		// -------------------------------------------------------------

		matColliderL_W = hCollider.geometryTransformMatrix();
		matColliderW_L = matColliderL_W.inverse();

		// -------------------------------------------------------------
		// create intersector for collider 
		// -------------------------------------------------------------

		intersector.create(oCollider, matColliderL_W);

		/////////////start collider base deformation////////////

		//create collider bounding box
		MBoundingBox boxCollider;
		tbb::parallel_for(tbb::blocked_range<unsigned int>(0, vtxCount),
			[&](tbb::blocked_range<unsigned int> r)
		{
			for (unsigned int k = r.begin(); k < r.end(); k++)
			{
				boxCollider.expand(colliderPnts[k]);
			}
		});

		if (boxCollider.intersects(boxDeformer))
		{
			//base collide deformer
			tbb::parallel_for(tbb::blocked_range<unsigned int>(0, vtxCount),
				[&](tbb::blocked_range<unsigned int> r)
			{
				for (unsigned int k = r.begin(); k < r.end(); k++)
				{
					float weight = weightValue(data, geoIndex, k);

					MPoint deformerWorldPnt = finalPnts[k] * matDeformerL_W;

					if (boxCollider.contains(deformerWorldPnt) == true)
					{
						if(weight !=0 )
						{
							//get the world space normal for the deformer vertex
							MVector temp_deformerNormal = deformerNormals[k];
							temp_deformerNormal *= matDeformerL_W;
							MFloatVector deformerWorldNormal(temp_deformerNormal);
							MFloatPointArray hitPoints;

							/*
							fnCollider.allIntersections(const MFloatPoint & 	raySource,
							const MFloatVector & 	rayDirection,
							const MIntArray * 	faceIds,
							const MIntArray * 	triIds,
							bool 	idsSorted,
							MSpace::Space 	space,
							float 	maxParam,
							bool 	testBothDirections,
							MMeshIsectAccelParams * 	accelParams,
							bool 	sortHits,
							MFloatPointArray & 	hitPoints,
							MFloatArray * 	hitRayParams,
							MIntArray * 	hitFaces,
							MIntArray * 	hitTriangles,
							MFloatArray * 	hitBary1s,
							MFloatArray * 	hitBary2s,
							float 	tolerance = 1e-6,
							MStatus * 	ReturnStatus = NULL
							)
							*/

							fnColliderTrans.allIntersections(deformerWorldPnt,
								deformerWorldNormal,
								NULL,
								NULL,
								NULL,
								MSpace::kWorld,
								1000000.00000,
								false,
								NULL,
								NULL,
								hitPoints,
								NULL,
								NULL,
								NULL,
								NULL,
								NULL,
								1e-6,
								&status
							);
							//final hit test, if hit, set has collision to ture;
							if (hitPoints.length() % 2 == 1)
							{

								MPointOnMesh pntOnMesh;
								MPoint temp_deformerWorldPnt(deformerWorldPnt);
								intersector.getClosestPoint(temp_deformerWorldPnt, pntOnMesh, 1000);
								MPoint collisionPoint = pntOnMesh.getPoint();
								collisionPoint *= matColliderL_W;

								MVector displacement = collisionPoint - temp_deformerWorldPnt;
								if (maxDisplacement < displacement.length())
									maxDisplacement = displacement.length(); //in world space
								temp_deformerWorldPnt = collisionPoint;

								MPoint finalPnt = temp_deformerWorldPnt * matDeformerW_L;
								finalPnts.set(finalPnt, k);
								hasCollision = true;
							}

						}
					}
				}
			});
			
			//bulge effect
			if (hasCollision == true && bulgeVal != 0)
			{

				tbb::parallel_for(tbb::blocked_range<unsigned int>(0, vtxCount),
					[&](tbb::blocked_range<unsigned int> r)
				{

					for (unsigned int k = r.begin(); k < r.end(); k++)
					{
						float weight = weightValue(data, geoIndex, k);
						//MFloatVectorArray finalNormals;
						//fnDeformer.getVertexNormals(true, finalNormals);
						MPoint worldPnt = finalPnts[k] * matDeformerL_W;
						MFloatVectorArray finalNormals;
						fnDeformer.getVertexNormals(true, finalNormals);
						MVector updatedNormal = finalNormals[k];
						updatedNormal *= matDeformerW_L;

						//get the distance from deformer worldPnt to collider surface
						//so we can used it as x to get the collider-surface-related 
						//offset value for each vertex.
						if (weight != 0)
						{
							MPointOnMesh pntOnCollider;
							intersector.getClosestPoint(worldPnt, pntOnCollider, 10000);
							MPoint cloestPnt = pntOnCollider.getPoint();

							cloestPnt *= matColliderL_W;

							double distance = cloestPnt.distanceTo(worldPnt);
							float value;
							float position = distance / (radius + 0.00001f);
							if (position < 0 || position >1)
								position = 1;

							float normalizedMaxD = maxDisplacement / (radius + 0.00001f);
							if (normalizedMaxD < 0 || normalizedMaxD >1)
								normalizedMaxD = 1;

							bulgeShape.getValueAtPosition(position, value);
							MVector bulgeVector = updatedNormal.normal() * value * normalizedMaxD * (bulgeVal);
							finalPnts.set(finalPnts[k] + bulgeVector * weight, k);
						}
					}
				});

			}
			
		}

		// -------------------------------------------------------------
		// updated deformer bounding box for next computation
		// -------------------------------------------------------------
		boxDeformer.clear();
		for (unsigned int i = 0; i < finalPnts.length(); i++)
		{
			MFloatPoint deformerWorldPnts = finalPnts[i] * matDeformerL_W;
			boxDeformer.expand(deformerWorldPnts);
		}


}

	iterGeo.setAllPositions(finalPnts);


	return MS::kSuccess;


}







