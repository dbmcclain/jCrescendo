# jCrescendo
Accurate Timbral Restoration of Music for Impaired Hearing
---
![image](https://github.com/dbmcclain/jCrescendo/assets/3160577/1803032e-a854-41f9-bc40-1605bfdead1e)

Sensioneural hearing impairment, most commonly caused by aging, noise exposure, over-loud music exposure, etc., affects mostly the higher frequencies of our hearing. That's because those higher frequencies are sensed by cochlear hair cells nearest to the entry window of the cochlea, and hence exposed to the most damaging levels of sound as it gradually dissipates along the length of the cohclea.

But our hearing is the product of a complex feedback control system comprised of the outer ear, middle ear, cochlea, afferent 8th nerve, spinal chord, brainstem, and higher brain function processing, feeding back to the middle ear and cochlea via the efferent 8th nerve. Our musical hearing arises from the whole of this processing, not just the cochlea.

jCrescendo is the result of more than 20 years of musical hearing research in my laboratory. The "j" of jCrescendo comes from our having used JUCE to produce AudioUnits and VST3 plugins for use in any audio processing application, such as DAWs or intercepting HAL layer processors such as Rogue Amoeba's SoundSource (https://rogueamoeba.com/soundsource/). The DSP processing found in our Crescendo algorith is independent of JUCE. If you download and install JUCE for yourself (https://juce.com), then the code found here should compile directly on any Mac computer for your own use. Otherwise, feel free to discard the two JUCE-centric files and use the DSP processing in your own way.

For most people, simply setting the Brighten control in the lower left is all that you need. Adjust it for yourself until it sounds best to you, then just leave it alone while you stream music and audio through its processing.

For those wanting more rigor, you can freely adjust each of the standard audiology bands to suit yourself. Crescendo is also the only known algorithm that can compensate for the ancillary condition of Hyper-Recruitment (HyperAcusis) and Decruitment. Some people may have a mixture of normal hearing in Bass, normal recruitment in midband frequencies, Hyper-Recruitment near narrowly damaged bands, and Decruitment at the very highest frequencies. 

The number displayed for Brighten corresponds to the dBHL threshold elevation of your hearing at 4 kHz. But all of Crescendo's corrections are performed in loudness space (Phons measure) and do not directly correspond to anything you could accomplish with audio processors such as compressors and equalizers.

The proper scientific setup for Crescendo has to determine the dBSPL (digital amplitude) corresponding to some measured sound intensity dBSPL. For calibration, set Crescendo to Bypass, then send a 1 kHz tone through your system and measure both the dBSPL (dBSPL Cal setting), and the dBFS (dBFS Cal setting). During setup, send e.g, a -18 dBFS 1 kHz tone and adjust the amplifier to produce 77-83 dBSPL. Record whatever settings you actually used in the Cal settings. Once calibrated, make all further volume level adjustment ahead of Crescendo so that it can know what you are hearing.

Ideally, Crescendo wants to play into spectrally flat output transducers. Make all coloration EQ adjustments ahead of Crescendo processing. If you have strong impairment you may find that you need to attenuate the output from Crescendo to avoid clipping distortion. Make up for that digital attenuation by increasing the output level at your audio amplifier. For myself, having moderately severe impairment, I routinely attenuate by 18 dB. That provides sufficient headroom for nearly all music. I run my audio studio with a nominal -18 dBFS level for the 0 dBVU average loud but comfortable level. 

After calibration, reduce the volume level (ahead of Crescendo) to whatever loudness level you find most comfortable. Crescendo will automaticall compensate for the decreased volume levels.

Crescendo can be though of as an 11-channel nonlinear audio compressor with each band having a different threshold level, dependent on your hearing. We find that most people show a 3.25 dB/zBark slope to their hearing impairment. So once you know what works at 4 kHz, it is easy to compute the settings needed for the other bands. That's exactly what the Bright control does, and as you move it, the volume sliders in each band move in concert with it. After finding tbe best Brighten level for yourself, feel free to make finer adjustments to any of the individual bands. The +D/-H adjustment below each slider is for compensation of HyperRecruitment and Decruitment.

The settings for Release Fast (34 ms) and Releas Slow (155 ms) was found by one of our users who is a professional musician and audio mixing engineer. Those settings, he felt, provided the most accurate rendition of music. Feel free to adjust these to taste. The Attack of the compression is internally fixed to 2 ms, with a 2 ms lookahead and a 10 ms hold.
