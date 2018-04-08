import numpy as np
import psana

class jungfrau (object) :

    def __init__ ( self ) :
        self.m_src1 = self.configSrc('source1')
        self.key_out1 = self.configStr('key_out1')
        self.m_src2 = self.configSrc('source2')
        self.key_out2 = self.configStr('key_out2')

    def beginjob( self, evt, env ) :
        # can use either the "full name" or the "alias" here
        # type "detnames exp=xpptut15:run=410" to see the list of names
        #self.det = psana.Detector('MfxEndstation.0:Jungfrau.0',env)
        self.det1 = psana.Detector('Jungfrau1M',env)
        self.det2 = psana.Detector('Epix100a',env)
        #self.det = psana.Detector(self.m_src,env)
        pass
 
    def beginrun( self, evt, env ) :
        pass

    def event( self, evt, env ) :
    
        # Jungfrau
        try: 
            #img1= self.det.image(evt) #assembled
            img1= self.det1.calib(evt) #for 3D unassembled
            #print 'Jungfrau image shape: ', img.shape

            # Put nothing in the event store if no image
            # evt.get() will then return fail, trapped in C code
            if img1 is None:
                #print 'Jungfrau image not found'
                return

            # Put back in the event store
            evt.put(img1, self.m_src1, self.key_out1)

        # Just return if any of the above fails
        except:
            return



        # Epix
        try:
            #img1= self.det.image(evt) #assembled
            img2= self.det2.calib(evt) #for 3D unassembled
            print 'Epix image shape: ', img2.shape

            # Put nothing in the event store if no image
            # evt.get() will then return fail, trapped in C code
            if img2 is None:
                #print 'Jungfrau image not found'
                return
            # Put back in the event store
            evt.put(img2, self.m_src2, self.key_out2)
        # Just return if any of the above fails
        except:
            return


        #for k in evt.keys():
        #    if 'jungfrau' in str(k): print k

    def endjob( self, evt, env ) :
        pass
