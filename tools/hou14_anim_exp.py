import math
import hou
import json
from collections import OrderedDict

class ChTypes:
    COMMON = 0
    EULER = 1
    QUATERNION = 2


class RotOrder:
    XYZ = 0

class Expression:
    CONSTANT = 0
    LINEAR = 1
    CUBIC = 2
    QLINEAR = 3
    

def parse_ch_name(name):
    n = name.split('/')
    n = n[-1].split(':')
    return n


def parse_ch_type(typeId):
    if typeId == 0:
        return ChTypes.COMMON
    elif typeId == 1:
        return ChTypes.EULER
    else:
        raise Exception("Uknonwn channel type {0}".format(typeId))


def parse_ch_rOrd(rOrd):
    d = RotOrder.__dict__
    r = rOrd.upper()
    if r in d:
        return d[r]
    else:
        raise Exception("Unknown rOrd {0}".format(rOrd))


def parse_kfr_expression(expr):
    if expr == "linear()": return Expression.LINEAR
    elif expr == "cubic()": return Expression.CUBIC
    elif expr == "qlinear()": return Expression.QLINEAR
    elif expr == "constant()": return Expression.CONSTANT
    else:
        print "Unknown keyframe expression {0}, will use CONSTANT".format(expr)
        return Expression.CONSTANT


def parm_name(prefix, idx):
    return "%s%d" % (prefix, idx)


class Component:
    class Keyframe:
        def __init__(self, k):
            self.kfr = k
            self.val = k.value()
            self.frame = k.frame()
            if k.isSlopeUsed():
                self.outSlope = k.slope()
                if k.isSlopeTied():
                    self.inSlope = self.outSlope
                else:
                    self.inSlope = k.inSlope()
            else:
                self.inSlope = 0.0
                self.outSlope = 0.0

            fps = hou.fps()
            self.inSlope /= fps
            self.outSlope /= fps

            expr = k.expression()
            self.expr = parse_kfr_expression(expr)
            # print expr, self.expr

    def __init__(self):
        self.keyframes = []
        pass

    def make_common(self, parm):
        self.parm = parm
        kfr = parm.keyframes()

        for k in kfr:
            c = Component.Keyframe(k)
            # if toRad:
            #     c.val = math.radians(c.val)
            if self.keyframes:
                prev = self.keyframes[-1]
                segmentLength = c.frame - prev.frame
                c.inSlope *= segmentLength
                c.outSlope *= segmentLength
            self.keyframes.append(c)

        if len(self.keyframes) > 1:
            k0 = self.keyframes[0]
            k1 = self.keyframes[1]
            segmentLength = k1.frame - k0.frame
            c.inSlope *= segmentLength
            c.outSlope *= segmentLength

    def __getitem__(self, idx):
        return self.keyframes[idx]

    def save_json(self):
        res = []
        for k in self.keyframes:
            res.append([k.frame, k.val, k.inSlope, k.outSlope])
        return res


class Channel:
    def __init__(self, node, idx):
        name = node.parm(parm_name("name", idx)).eval()
        typeId = node.parm(parm_name("type", idx)).eval()
        rOrd = node.parm(parm_name("rOrd", idx)).evalAsString()
        size = node.parm(parm_name("size", idx)).eval()
        valParm = node.parmTuple(parm_name("value", idx))

        n = parse_ch_name(name)
        self.name = n[0]
        self.subName = n[1] if len(n) > 1 else ""
        self.type = parse_ch_type(typeId)
        self.rOrd = parse_ch_rOrd(rOrd)

        if self.type == ChTypes.EULER:
            self.size = 3
            # toRad = True
            self._load_euler_quat(valParm, rOrd)
            # self._load_euler()
        else:
            self.size = size
            self._load(valParm)

    def _load(self, valParm):
        components = []
        for i in xrange(0, self.size):
            c = Component()
            c.make_common(valParm[i])
            components.append(c)

        if self.size > 0:
            self._set_components(components)

    def _set_components(self, components):
        for c in components:
            if not c:
                raise Exception("Empty component of channel %s" % (self.name))

        lastFrame = 0.0
        expr = components[0][0].expr
        for c in components:
            for k in c:
                if expr != k.expr:
                    print("Component's expression differ, will use default")
                    k.expr = expr
                lastFrame = max(k.frame, lastFrame)

        # print '-----', expr

        self.expr = expr
        self.components = components
        self.lastFrame = lastFrame
        # print self.name, self.subName, lastFrame

    def _load_euler_quat(self, valParm, rOrd):
        self.type = ChTypes.QUATERNION

        frames = []
        for i in xrange(0, self.size):
            for k in valParm[i].keyframes():
                frames.append(k.frame())

        frames = sorted(set(frames))

        components = []
        self.size = 4
        for i in xrange(0, self.size):
            components.append(Component())

        for frame in frames:
            v = [valParm[i].evalAtFrame(frame) for i in xrange(0, 3)]
            q = hou.Quaternion()
            q.setToEulerRotates(v, rOrd)

            for i in xrange(0, 4):
                c = components[i]
                kq = hou.Keyframe()
                kq.setFrame(frame)
                kq.setValue(q[i])
                kq.setExpression("qlinear()", hou.exprLanguage.Hscript)
                c.keyframes.append(Component.Keyframe(kq))

        self._set_components(components)

    def save_json(self):
        res = OrderedDict()
        res['name'] = self.name
        res['subName'] = self.subName
        res['type'] = self.type
        res['rord'] = self.rOrd
        res['expr'] = self.expr
        res['size'] = self.size
        res['lastFrame'] = self.lastFrame
        comp = []
        for c in self.components:
            comp.append(c.save_json())
        res['comp'] = comp
        return res


class Animation:
    def __init__(self, node):
        numChannels = node.parm("numchannels").eval()

        channels = []
        lastFrame = 0.0
        for i in xrange(0, numChannels):
            c = Channel(node, i)
            lastFrame = max(c.lastFrame, lastFrame)
            channels.append(c)

        self.name = node.name()
        self.channels = channels
        self.lastFrame = lastFrame

    def save_json(self, filepath):
        res = OrderedDict()
        res['name'] = self.name
        res['lastFrame'] = self.lastFrame
        ch = []
        for c in self.channels:
            ch.append(c.save_json())
        res['channels'] = ch

        print "Saving to %s" % (filepath,)
        with open(filepath, 'w') as f:
            json.dump(res, f, indent=4, separators=(',', ': '))
            # json.dump(res, f, separators=(',', ':'))


def main():
    # chNode = hou.node('/obj/ANIM/MOT/test')
    chNode = hou.node('/obj/ANIM/MOT/c116_0610_wait_accent_b')
    anim = Animation(chNode)
    anim.save_json(r"w:\houdini\reversed\cot\c116_ev.pak\test.anim")


if __name__ in ['__main__', '__builtin__']:
    main()
    pass
