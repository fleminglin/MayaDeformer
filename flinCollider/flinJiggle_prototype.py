import maya.api.OpenMaya as om
import sys

def maya_useNewAPI():
    pass

class flinJiggle( om.MPxNode ):
	kPluginNodeId = om.MTypeId(0x00000123)
	kPluginName = 'flinJiggle'

	aGoal      =  om.MPoint()
	aOutput    =  om.MPoint()

	aDamping   =  om.MObject()
	aStiffness =  om.MObject()
	aTime      =  om.MObject()
	aJiggle    =  om.MObject()
	aParentInverse = om.MObject()
	aGoalInverse = om.MObject

	def __init__(self):
		om.MPxNode.__init__(self)
		
		self._initialize = False
		self._currentPosition = om.MFloatVector()
		self._previousPosition = om.MFloatVector()

		self._currentTime = om.MObject()
		self._previousTime = om.MObject()

	def compute(self, plug, data):
			if plug == self.aOutput:

				#pull data from datablock
				#
				goal        = 	data.inputValue(self.aGoal).asFloatVector()
				damping     = 	data.inputValue(self.aDamping).asFloat()
				stiffness   = 	data.inputValue(self.aStiffness).asFloat()
				time 	    = 	data.inputValue(self.aTime).asFloat()
				jiggle      =   data.inputValue(self.aJiggle).asFloat()
				parentInverse = data.inputValue(self.aParentInverse).asFloatMatrix()
				goalInverse =	data.inputValue(self.aGoalInverse).asFloatMatrix()

				goal *= goalInverse

				#initialzie position
				#
				if not self._initialize:
					self._currentPosition  =  goal 
					self._previousPosition =  goal 
					#self._currentTime = time
					#self._previousTime = time
					self._initialize = True

				#time_difference = self._currentTime - self._previousTime

				#algorithm
				#
				velocity      =    (self._currentPosition - self._previousPosition) * (1-damping) 
				new_position  =    self._currentPosition + velocity 
				goal_force    =    (goal - new_position) * stiffness 
				new_position +=    goal_force  

				#update positon
				#
				self._previousPosition = self._currentPosition
				self._currentPosition  = new_position 
				#self._previousTime = self._currentTime
				#self._currentTime = time

				#additional jiggle 
				#
				new_position = goal + (new_position - goal) * (1 + jiggle)
				new_position *= parentInverse

				#set output
				#
				hOutput = data.outputValue(self.aOutput)
				hOutput.setMFloatVector( new_position )
				data.setClean(plug)

#creator (pointer for maya )
#
def creator():
	return flinJiggle()

#initialize node attribute 
#
def initialize():
	nAttr = om.MFnNumericAttribute()
	uAttr = om.MFnUnitAttribute()
	mAttr = om.MFnMatrixAttribute()

	flinJiggle.aOutput = nAttr.createPoint( 'output', 'output' )
	nAttr.keyable = False
	nAttr.storable = False
	om.MPxNode.addAttribute( flinJiggle.aOutput )

	flinJiggle.aDamping = nAttr.create('damping', 'damping', om.MFnNumericData.kFloat, 0.0 )
	nAttr.keyable = True
	nAttr.readable = True
	nAttr.storable = True
	nAttr.setMin(0)
	nAttr.setMax(1)
	flinJiggle.addAttribute( flinJiggle.aDamping )

	flinJiggle.aStiffness = nAttr.create('stiffness', 'stiffness', om.MFnNumericData.kFloat, 0.0 )
	nAttr.keyable = True
	nAttr.readable = True
	nAttr.storable = True
	nAttr.setMin(0)
	nAttr.setMax(1)
	flinJiggle.addAttribute( flinJiggle.aStiffness )

	flinJiggle.aJiggle = nAttr.create('jiggle', 'jiggle', om.MFnNumericData.kFloat, 0.0 )
	nAttr.keyable = True
	nAttr.readable = True
	nAttr.storable = True
	flinJiggle.addAttribute( flinJiggle.aJiggle )


	flinJiggle.aTime = uAttr.create('inTime', 'inTime', om.MFnUnitAttribute.kTime)
	uAttr.keyable = True
	uAttr.readable = True
	uAttr.storable = True
	flinJiggle.addAttribute( flinJiggle.aTime )

	flinJiggle.aGoal = nAttr.createPoint( 'goal', 'goal' )
	nAttr.keyable = True
	nAttr.readable = True
	nAttr.storable = True
	flinJiggle.addAttribute( flinJiggle.aGoal )

	flinJiggle.aParentInverse = mAttr.create('parentInverse', 'parentInverse', om.MFnData.kMatrix)
	flinJiggle.addAttribute( flinJiggle.aParentInverse )

	flinJiggle.aGoalInverse = mAttr.create('goalInverse', 'goalInverse', om.MFnData.kMatrix)
	flinJiggle.addAttribute( flinJiggle.aGoalInverse )

	flinJiggle.attributeAffects( flinJiggle.aGoal, flinJiggle.aOutput )
	flinJiggle.attributeAffects( flinJiggle.aDamping, flinJiggle.aOutput )
	flinJiggle.attributeAffects( flinJiggle.aStiffness, flinJiggle.aOutput )
	flinJiggle.attributeAffects( flinJiggle.aTime, flinJiggle.aOutput )
	flinJiggle.attributeAffects( flinJiggle.aJiggle, flinJiggle.aOutput )
	flinJiggle.attributeAffects( flinJiggle.aParentInverse, flinJiggle.aOutput )
	flinJiggle.attributeAffects( flinJiggle.aGoalInverse, flinJiggle.aOutput )


#initialize Plugin
#
def initializePlugin(obj):
	fnPlugin = om.MFnPlugin(obj, 'flin', '1.0', 'Any')
	try:
		fnPlugin.registerNode(flinJiggle.kPluginName, flinJiggle.kPluginNodeId, creator, initialize)
	except:
		sys.stderr.write("Failed to register command\n")
		raise Exception

#uninitialize Plugin
#
def uninitializePlugin(obj):
	fnPlugin = om.MFnPlugin(obj)
	try:
		fnPlugin.deregisterNode(flinJiggle.kPluginNodeId)
	except:
		sys.stderr.write("Failed to deregister command\n")
		raise Exception

