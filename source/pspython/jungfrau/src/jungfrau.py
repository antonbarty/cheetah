import numpy as np
import psana

class jungfrau (object) :

    def __init__ ( self ) :
        self.m_src = self.configSrc('source')
        self.key_out = self.configStr('key_out')

    def beginjob( self, evt, env ) :
        # can use either the "full name" or the "alias" here
        # type "detnames exp=xpptut15:run=410" to see the list of names
        self.det = psana.Detector('Jungfrau1M',env)
        pass
 
    def beginrun( self, evt, env ) :
        pass

    def event( self, evt, env ) :
        #img = self.det.image(evt) #assembled
        img = self.det.calib(evt) #for 3D unassembled
        if img is None:
            print 'jungfrau not found'
            return
        #print 'Found jungfrau with shape',img.shape,img
        evt.put(img,self.m_src,self.key_out)
        #for k in evt.keys():
        #    if 'jungfrau' in str(k): print k

    def endjob( self, evt, env ) :
        pass
