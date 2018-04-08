import numpy as np
import psana

class jungfrau (object) :

    def __init__ ( self ) :
        self.m_src = self.configStr('source')
        self.key_out = self.configStr('key_out')

    def beginjob( self, evt, env ) :
		# can use either the "full name" or the "alias" here
		# type "detnames exp=xpptut15:run=410" to see the list of names
		self.det = psana.Detector('MfxEndstation.0:Jungfrau.0',env)
		#self.det = psana.Detector('Jungfrau1M',env)
        #self.det = psana.Detector(self.m_src,env)
		pass
 
    def beginrun( self, evt, env ) :
        pass

    def event( self, evt, env ) :
    
		# Handle cleanly cases where the image doesn't return
		try: 
			#img = self.det.image(evt) #assembled
			img = self.det.calib(evt) #for 3D unassembled
			#print 'Jungfrau image shape: ', img.shape

			# Put nothing in the event store if no image
			# evt.get() will then return fail, trapped in C code
			if img is None:
				#print 'Jungfrau image not found'
				return

			# Put back in the event store
			evt.put(img,self.m_src,self.key_out)

		# Just return if any of the above fails
		except:
			return

		#for k in evt.keys():
		#    if 'jungfrau' in str(k): print k

    def endjob( self, evt, env ) :
        pass
