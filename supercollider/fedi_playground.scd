// generate a slurp-sound when above optimal rpm
(
~buf_slurp = Buffer.read(s, "/Users/jhammers/slurp-sounds/slurp_cont.wav");
~bus_slurp = Bus.audio(s, 2);

~slurp_play = {|loop=1, startPos=0| // startPos in seconds
	~slurp = {
		Out.ar(~bus_slurp,
			PlayBuf.ar(2, ~buf_slurp, BufRateScale.kr(~buf_slurp), // * MouseY.kr(0.5,1.5),
			startPos: BufSampleRate.kr(~buf_slurp) * startPos,
			loop: loop));
	}.play;
};
~slurp_lpf_play = {
	~slurp_lpf = {|f=15000,m=2| // f:[200,15000], m:[0,5] (oder so ..)
		LPF.ar(~bus_slurp.ar, f, m) //*1.5)
	}.play;
};
~slurp_stop = {
	~set_rpm_handler.value;
	if (~slurp != nil, {~slurp.free; ~slurp = nil});
	if (~slurp_lpf != nil, {~slurp_lpf.free; ~slurp_lpf = nil});
};
~slurp_start = {
	~slurp_stop.value;
	~slurp_lpf_play.value;
	OSCdef(\msg_rpm, {|m|
		~rpm_handler.value(m[1]); // still control the engine-sound!
		~slurp_lpf.set(\m, m[1].linlin(0.38,0.65, 0,1.2));
		//~lpf.set(\f, m[1].linlin(0.38,1, 400,15000));
		//m[1].postln;
	}, '/rpm');
	~slurp_play.value;
};
OSCdef(\msg_slurp_start, {|m|
	"slurp_start".postln;
	~slurp_start.value;
}, '/slurp_start');
OSCdef(\msg_slurp_stop, {|m|
	"slurp_stop".postln;
	~slurp_stop.value;
}, '/slurp_stop');


// map fuel consumption on pitched noise
~pitch_stop = {
	//if (~pitch_control_window != nil, {~pitch_control_window.close; ~pitch_control_window = nil});
	if (~pitch != nil, {~pitch.free; ~pitch = nil});
};
~pitch_show_controls = {
	{~pitch_control_window = ~control_gui2.value([
		[~pitch, \f, [300,3000], "freq"],
		[~pitch, \bw, [0.01, 0.6], "bandwidth"],
		[~pitch, \a, [0.4,5], "ampl."],
	], false)}.defer(0.2);
};
OSCdef(\msg_pitch_show_controls, {|m|
	~pitch_show_controls.value;
}, '/pitch_show_controls');
OSCdef(\msg_pitch_stop, {|m|
	"pitch_stop".postln;
	~pitch_stop.value;
}, '/pitch_stop');
OSCdef(\msg_pitch_start, {|m|
	"pitch_start".postln;
	~pitch_stop.value;
	~pitch = {|f=300,bw=0.1,a=1| {a*Resonz.ar(WhiteNoise.ar(0.5), f, bw)}!2 }.play;
	OSCdef(\msg_L_100km, {|m| // range [0-40?]
		if(m[1].isNaN.not && (m[1] != inf) && (~pitch != nil), {
			~pitch.set(\f, m[1].linlin(0,40,300,3000));
		});
	}, '/L_100km');
}, '/pitch_start');
"FEDIs initialized!"
)


~pitch_control_window
if (~pitch_control_window != nil, {~pitch_control_window.close});


// same as below, but the impulses for the grains come directly from the program
(
~sound_control_bus = Bus.control(s, 3); // control bus for controlling the triggered sound
~sound_control = {|f=300, bw=0.1, a=1| // synth writing on the control bus
	Out.kr(~sound_control_bus, [f, bw, a])
}.play;
~triggered_sound = { // the sound that gets triggered
	var p = ~sound_control_bus.getnSynchronous(3);
	{LPF.ar(WhiteNoise.ar(p[2]), p[0])
		* EnvGen.ar(Env.perc(0.001,0.5), doneAction:2)}!2;
};
OSCdef(\msg_consumption_tick, {|m|
	//~freq = m[1]; ...
	~triggered_sound.play() //args: [\f, ~freq])
}, '/consumption_tick');
{~control_gui2.value([
	[~onset_input, \f, [0,3], "freq"],
	[~sound_cssssssssontrol, \f, [200, 2000], "PF freq"],
	[~sound_control, \bw, [0.01, 0.6], "BPF bandwidth"],
	[~sound_control, \a, [0.4,5], "amplitude"],
], false)}.defer(0.2);
/*OSCdef(\msg_L_100km, {|m| // range [0-40?] // this is optional!
	if(m[1].isNaN.not && (m[1] != inf), {
		~sound_control.set(\f, m[1].linlin(0,40,300,2000));
	});
}, '/L_100km');*/
)

// map (absolute) fuel consumption on the frequency of grains
// optionally with modifying the cutoff freq based on l/100km
(
~onset_input_bus = Bus.audio(s); // input for the impulses
~onset_bus = Bus.control(s); // control bus receiving the trigger
~onset_trigger = { // convert data from the control-bus to trigger signals
	SendTrig.kr(~onset_bus.kr);
}.play;
~onset_detect = { |threshold = 0.2, floor = 0.15| // impulses => trigger control bus
	var d = Buffer.alloc(s, 512);
	var detect = Onsets.kr(FFT(d, ~onset_input_bus.ar), threshold, mingap: 0.1, floor: floor);
	Out.kr(~onset_bus, detect);
}.play;
~sound_control_bus = Bus.control(s, 3); // control bus for controlling the triggered sound
~sound_control = {|f=300, bw=0.1, a=1| // synth writing on the control bus
	Out.kr(~sound_control_bus, [f, bw, a])
}.play;
~triggered_sound = { // the sound that gets triggered
	var p = ~sound_control_bus.getnSynchronous(3);
	{LPF.ar(WhiteNoise.ar(p[2]), p[0])
		* EnvGen.ar(Env.perc(0.001,0.5), doneAction:2)}!2;
};
/*~triggered_sound2 = {
	var sc = ~sound_control_bus.getnSynchronous(3);
	Resonz.ar(WhiteNoise.ar(sc[2]), sc[0], sc[1])
	* EnvGen.ar(Env.perc(0.001,0.5), doneAction:2);
};*/
~onset_input = {|f=2| // generating the impulses
	Out.ar(~onset_input_bus, Impulse.ar(f));
}.play;
~trigger_func.free;
~trigger_func = OSCFunc({~triggered_sound.play},'/tr', s.addr);
{~control_gui2.value([
	[~onset_input, \f, [0,3], "freq"],
	[~sound_control, \f, [200, 2000], "PF freq"],
	[~sound_control, \bw, [0.01, 0.6], "BPF bandwidth"],
	[~sound_control, \a, [0.4,5], "amplitude"],
], false)}.defer(0.2);

OSCdef(\msg_ml_sec, {|m| // range: [0-5]
	~onset_input.set(\f, m[1].linlin(0,5,0,1.5));
}, '/ml_sec');
OSCdef(\msg_L_100km, {|m| // range [0-40?] // this is optional!
	if(m[1].isNaN.not && (m[1] != inf), {
		~sound_control.set(\f, m[1].linlin(0,40,300,2000));
	});
}, '/L_100km');
)


(
//only testing (slurp)
{LPF.ar(~bus_slurp.ar,
	MouseX.kr(200, 8000),
	MouseX.kr(0,5)
)}.play;
//{~bus_slurp.ar}.play;
/*	{
		~control_gui2.value([[~lpf, \f, [200,10000], "frequency"],
							//[~lpf, \m, [-20,15], "mul"]
							[~lpf, \m, [0,5], "mul"]
						], false);
	}.defer(0.2);*/
~play.value;

)
//~slurp.free;

/////// TESTING /////////

// (
// n=LFNoise1;
// Ndef(\x,{|f=65|
// 	a=SinOsc.ar(f, Ndef(\x).ar*n.ar(0.1,3), n.ar(3,6)).tanh;
// 	9.do{
// 		a=AllpassL.ar(a,0.3,{0.2.rand+0.1}!2,5)
// 	};
// 	a.tanh
// }).play
// )
//
// (
// n=LFNoise1;
// Ndef(\x,{|f=100|
// 	a=SinOsc.ar(f, Ndef(\x).ar*n.ar(0.1,3), n.ar(3,1)).tanh;
// 	9.do{
// 		a=AllpassL.ar(a,0.3,{0.2.rand+0.1}!2,5)
// 	};
// 	a.tanh
// }).play;
// w = Ndef(\x);
// )
//
//
// play{a=HPF.ar(ar(PinkNoise,5e-3),10)*Line.kr(0,1,9);ar(GVerb,({|i|ar(Ringz,a*LFNoise1.kr(0.05+0.1.rand),55*i+60,0.2)}!99).sum,70,99).tanh}

//{ Klank.ar(`[[440, 980, 1220, 1560], nil, [2, 2, 2, 2]], WhiteNoise.ar(0.005)) }.play;
// (
// fork{
// 	100.do({
// 		{
// 			Klank.ar(`[[440, 980, 1220, 1560], nil, [2, 2, 1, 1]], WhiteNoise.ar(0.005))
// 			//LPF.ar(WhiteNoise.ar(1), MouseX.kr(200,20000, 1))
// 			* EnvGen.ar(Env.perc(0.01,1), doneAction:2)
// 		}.play;
// 		1.wait;
// 	});
// }
//)
