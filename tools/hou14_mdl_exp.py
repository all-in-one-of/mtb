import hou
import json

from pprint import pprint

def findAttrib(attribs, name):
    for a in attribs:
        if a.name() == name:
            return a
    return None

def jointNameFormPCaptPath(path):
    # 'j_220/cregion 0'
    return path.split('/')[0]



class Joint:
    def __init__(self, name, geoIdx, expIdx, skinIdx, parent):
        self.name = name
        self.geoIdx = geoIdx
        self.expIdx = expIdx
        self.skinIdx = skinIdx
        self.parent = parent
        assert((not parent) or (parent.expIdx < self.expIdx))

    def __repr__(self):
        #parName = self.parent.name if self.parent else "None"
        #parIdx = self.parent.expidx if self.parent else -1
        #return "Joint <%s>:%d, p <%s>:%d" % (self.name, self.idx, parName, parIdx)
        return "Joint <%s>:%d:%d:%d" % (self.name, self.geoIdx, self.expIdx, self.skinIdx)

class Rig:
    def __init__(self):
        pass

    def build(self, boneCaptureAttr, rootNode):
        self.geoJointsMap = {}
        self.extJointsMap = {}

        self._list_captured_joints(boneCaptureAttr)

        self.geoJointsList = [None] * len(self.geoJointsMap)
        self.expJointsList = []
        self.skinJointsList = []

        self._build_rig_from_hierarchy(rootNode)

    def get_skin_idx(self, geoIdx):
        if geoIdx < 0: return -1
        return self.geoJointsList[geoIdx].skinIdx        

    def _list_captured_joints(self, boneCaptureAttr):
        ippt = boneCaptureAttr.indexPairPropertyTables()[0]

        ipptSize = ippt.numIndices()
        for i in xrange(0, ipptSize):
            val = ippt.stringPropertyValueAtIndex('pCaptPath', i)
            jnt = jointNameFormPCaptPath(val)
            # print i, jnt
            self.geoJointsMap[jnt] = i
        pass

    def _build_rig_from_hierarchy(self, rootNode):
        def hie(node, func, parent=None):
            jParent = func(node, parent)
            for c in node.outputs():
                hie(c, func, jParent)

        def f(node, parent):
            name = node.name()
            geoIdx = -1
            skinIdx = -1

            if name in self.geoJointsMap:
                geoIdx = self.geoJointsMap[name]

                skinIdx = len(self.skinJointsList)
                self.skinJointsList.append(None)

            expIdx = len(self.expJointsList)
            self.expJointsList.append(None)
            self.extJointsMap[name] = expIdx

            j = Joint(name, geoIdx, expIdx, skinIdx, parent)
            self.expJointsList[expIdx] = j
            if geoIdx != -1:
                self.geoJointsList[geoIdx] = j
                self.skinJointsList[skinIdx] = j

            j.mtx = node.parmTransform()
            j.imtx = node.worldTransform().inverted()

            return j

        hie(rootNode, f)

    def save_json(self, filepath):
        joints = []
        mtx = []
        
        for j in self.expJointsList:
            name = j.name
            idx = j.expIdx
            parIdx = j.parent.expIdx if j.parent else -1
            skinIdx = j.skinIdx
            joints.append({'name':name, 'idx':idx, 'parIdx':parIdx, 'skinIdx':skinIdx})
            mtx.append(j.mtx.asTuple())

        imtx = []
        for j in self.skinJointsList:
            assert(len(imtx) == j.skinIdx)
            imtx.append(j.imtx.asTuple())

        data = {'joints':joints, 'mtx':mtx, 'imtx':imtx}

        with open(filepath, 'w') as f:
            #json.dump(data, f)
            json.dump(data, f, separators=(',', ':'))
            #json.dump(data, f, indent=4, separators=(',', ': '))



def build_rig(geoNode, geo):
    boneCaptureAttr = geo.findPointAttrib("boneCapture")

    skelRoot = geo.attribValue('pCaptSkelRoot')
    restNode = geoNode.node(skelRoot)
    rootNode = restNode.node('root')

    rig = Rig()
    rig.build(boneCaptureAttr, rootNode)
    return rig

def save_rig(geoNode, geo, filepath):
    rig = build_rig(geoNode, geo)
    print 'Save rig to ', filepath
    #pprint(rig.expJointsList)
    rig.save_json(filepath)

def cook(node, geo):
    boneCaptureAttr = geo.findPointAttrib("boneCapture")
    if not boneCaptureAttr:
        return

    rig = build_rig(node, geo)
    
    defLen = 4

    jidxAttr = geo.addAttrib(hou.attribType.Point, "jidx", [0] * defLen, create_local_variable=False)
    jwgtAttr = geo.addAttrib(hou.attribType.Point, "jwgt", [0.0] * defLen, create_local_variable=False)

    for p in geo.points():
        bone = p.attribValue(boneCaptureAttr)

        jidx = [int(bone[i]) for i in xrange(0, len(bone), 2)]
        jwgt = [bone[i] for i in xrange(1, len(bone), 2)]

        for i in xrange(0, len(jidx)):
            skinIdx = rig.get_skin_idx(jidx[i])
            if skinIdx < 0:
                jidx[i] = 0
                jwgt[i] = 0.0
            else:
                jidx[i] = skinIdx

        for _ in xrange(len(jidx), defLen):
            jidx.append(0)
            jwgt.append(0)

        p.setAttribValue(jidxAttr, jidx)
        p.setAttribValue(jwgtAttr, jwgt)

    boneCaptureAttr.destroy()

    pass

def save(**kwargs):
    node = kwargs['node']

    parmRigFile = node.parm('parmRigFile')
    rigFile = parmRigFile.eval() if parmRigFile else None
    parmGeoFile = node.parm('parmGeoFile')
    geoFile = parmGeoFile.eval() if parmGeoFile else None
    
    if not (rigFile or geoFile):
        print 'no file to save'
        return

    node.cook()
    geo = node.geometry().freeze()
    geoPar = node.inputs()[0].geometry().freeze()

    if rigFile:
        #save_rig(node, geo, rigFile)
        save_rig(node, geoPar, rigFile)
    if geoFile:
        print 'Save geo to ', geoFile
        geo.saveToFile(geoFile)


def test_main():
    geoNode = hou.node('/obj/geo/capture1')
    geo = geoNode.geometry().freeze()
    save_rig(geoNode, geo, r"w:\houdini\reversed\cot\c116_ev.pak\test.rig")

if __name__ in ['__main__', '__builtin__']:
    #test_main()
    pass