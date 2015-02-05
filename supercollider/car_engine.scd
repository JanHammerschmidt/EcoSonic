(
//initialize server
s = Server.internal;
Server.default = s;
s.boot;
//s.volume = -10

// main output bus for engine
~engine_out = Bus.audio(s, 2);

~gear = 1; // gears from 1 to 6

~fft_buf = Buffer.alloc(s, 1024, 1);
~engine_loudness_out = Bus.control(s);

{
// define Engine-Synth
SynthDef(\engine, {

	| // arguments range: 0.0 - 1.0
	mixCylinders   = 0.8,
	mixParabolic   = 0.9,
	engineSpeed    = 0.1,
	parabolaDelay  = 0.15,
	warpDelay      = 0.4,
	waveguideWarp  = 0.67,
	wguideFeedback = 0.35,
	wguideLength1  = 0.2,
	wguideLength2  = 0.3,
	wguideWidth1   = 0.5,
	wguideWidth2   = 0.7
	|

	// To be able to send arrays as arguments you have to declare them as variables and
	// use NamedControl.kr. Take also a look at the MIDI example at the bottom how to address them.
	var transDelay = NamedControl.kr(\transDelay, [0.2, 0.3, 0.45]);
	var overtonePhase = NamedControl.kr(\overtonePhase, [0.25, 0.35, 0.5]);
	var overtoneFreq = NamedControl.kr(\overtoneFreq, [0.3, 0.47, 0.38]);
	var overtoneAmp = NamedControl.kr(\overtoneAmp, [0.1, 0.2, 0.2]);

	var noise, bufferA, bufferB, bufferTd, fourstroke, phasor, td, parabola, fm1, preFM1,
	fm2, preFM2, overtone, overtoneDrive, e1b, e2a, e2b, e1a, spacewarp, engine;

	//engineSpeed = MouseX.kr(0,1);


	bufferA = LocalBuf(44100, 1);
	bufferB = LocalBuf(44100, 1);
	bufferTd = LocalBuf(44100, 1);


	noise = WhiteNoise.ar;
	noise = OnePole.ar(noise, exp(-2pi * (20 * SampleDur.ir)));
	noise = OnePole.ar(noise, exp(-2pi * (20 * SampleDur.ir)));
	noise = (DelTapWr.ar([bufferA, bufferB], [noise * 0.5, noise * 30]));

	phasor = LFSaw.ar(
		OnePole.ar(K2A.ar(engineSpeed) * 30, exp(-2pi * (0.8 * SampleDur.ir))),
		1, 0.5, 0.5);
	td = DelTapWr.ar(bufferTd, phasor);

	fourstroke = DelTapRd.ar(bufferA, noise[0], [5, 10, 15, 20]/1000, 4);
	fourstroke = phasor + fourstroke - [0.75, 0.5, 0.25, 0];
	fourstroke = (fourstroke * 2pi).cos;
	fourstroke = fourstroke * (DelTapRd.ar(bufferB, noise[1], [5, 10, 15, 20]/1000, 4) + ((1 - engineSpeed) * 15 + 7));
	fourstroke = 1 / ((fourstroke * fourstroke) + 1);
	fourstroke = fourstroke.sum * mixCylinders;
	fourstroke = fourstroke - OnePole.ar(fourstroke, exp(-2pi * (4 * SampleDur.ir)));


	parabola = DelTapRd.ar(bufferTd, td, (parabolaDelay * 100)/1000, 1) - 0.5;
	parabola = parabola * parabola * (-4) + 1 * 3 * mixParabolic;


	preFM1 = DelTapRd.ar(bufferTd, td, (warpDelay * 100)/1000, 1);
	preFM1 = (preFM1 * 2pi).cos;
	preFM2 = K2A.ar(engineSpeed * waveguideWarp);
	preFM2 = OnePole.ar(preFM2, exp(-2pi * (0.2 * SampleDur.ir)));
	fm1 = (1 - preFM1) * preFM2 + 0.5;
	fm2 = (preFM2 * preFM1) + 0.5;


	overtoneDrive  = overtoneDrive!3;
	overtone = overtone!3;

	3.do{|i|

		overtoneDrive[i] = DelTapRd.ar(bufferTd, td, (transDelay[i]*100)/1000) * (0.5**(i+1)*32);
		overtoneDrive[i] = Wrap.ar(overtoneDrive[i]);

		overtone[i] = overtoneDrive[i].max(overtonePhase[i]) - overtonePhase[i];
		overtone[i] = overtone[i] * (1 - overtonePhase[i]).reciprocal;
		overtone[i] = overtone[i] * ((overtoneFreq[i] * 12) * overtonePhase[i]);
		overtone[i] = Wrap.ar(overtone[i]) - 0.5;
		overtone[i] = (overtone[i] * overtone[i]) * (-4) + 1 * 0.5;
		overtone[i] = (overtone[i] * (1 - overtoneDrive[i])) * (overtoneAmp[i] * 12);
	};


	# e1b, e2b, e2a, e1a = DelayC.ar(
		in: InFeedback.ar(bus:(10..13)),
		maxdelaytime: 1,
		delaytime: ((([wguideLength1,wguideWidth1,wguideLength2,wguideWidth2] * 40)
			* [fm1,fm1,fm2,fm1])/1000)
	);

	OffsetOut.ar(11, e1b + overtone[1]);

	e2b = e2b + overtone[2];
	OffsetOut.ar(13, e2b);

	e2a = e2a + overtone[0];
	OffsetOut.ar(10, e2a);

	OffsetOut.ar(12, e1a * wguideFeedback + (parabola - OnePole.ar(parabola, exp(-2pi * (30 * SampleDur.ir)))));

	spacewarp = e1b + e2b + e2a + e1a;
	spacewarp = spacewarp - OnePole.ar(spacewarp, exp(-2pi * (200 * SampleDur.ir)));
	spacewarp = spacewarp - OnePole.ar(spacewarp, exp(-2pi * (200 * SampleDur.ir)));

	engine = (spacewarp + fourstroke)!2 * 0.5;

	Out.ar(~engine_out, engine); // Out.ar(0
	//Out.ar(~engine_out, SinOsc.ar([330,440], 0, 0.4))

}).send(s);

// load sound files
~buf_honk = Buffer.read(s, "/Users/jhammers/Dropbox/Eigene Dateien/phd/code/car_simulator/media/honking/honk.wav");
~buf_flash = Buffer.read(s, "/Users/jhammers/Dropbox/Eigene Dateien/phd/code/car_simulator/media/flash/shutter.wav");

//start & stop engine
~start_engine = {
	"startEngine".postln;
	~engine.free;
	~engine_stream.free;
	~engine_loudness.free;
	~engine = Synth(\engine);
	~engine_stream = {
		Out.ar(0, ~engine_out.ar);
	}.play(addAction:\addToTail);
	~engine_loudness = {
		var fft = FFT(~fft_buf, ~engine_out.ar[0]);
		var sone = LPF.kr(Loudness.kr(fft), 1);
		var phon = 40 + 10 * log2(sone);
		Out.kr(~engine_loudness_out, phon);
	}.play(addAction:\addToTail);
	~set_rpm_handler.value;
};
~stop_engine = {
	"stopEngine".postln;
	~unset_rpm_handler.value;
	~engine.set(\engineSpeed, 0);
	{
		~engine.free;
		~engine = nil;
		~engine_stream.free;
		~engine_stream = nil;
		~engine_loudness.free;
		~engine_loudness = nil;
	}.defer(1);
};
// define OSC functions
OSCdef(\msg_startEngine, {|m|
	~start_engine.value;
}, '/startEngine');
OSCdef(\msg_stopEngine, {|m|
	~stop_engine.value;
}, '/stopEngine');
~rpm_handler = {|r|
	(~engine !== nil).if({
		~engine.set(\engineSpeed, r);
	});
};
~set_rpm_handler = {
	OSCdef(\msg_rpm, {|m|
		~rpm_handler.value(m[1]);
	}, '/rpm');
};
~unset_rpm_handler = {
	OSCdef(\msg_rpm, {|m|},'/rpm');
};
OSCdef(\msg_honk, {|m|
	{ PlayBuf.ar(2, ~buf_honk, BufRateScale.kr(~buf_honk)) }.play();
}, '/honk');
OSCdef(\msg_flash, {|m|
	{ PlayBuf.ar(1, ~buf_flash, BufRateScale.kr(~buf_flash))!2 }.play();
}, '/flash');
OSCdef(\msg_gear, {|m|
	~gear = m[1];
	//~gear.postln;
}, '/gear');

~control_gui2 = {|params, close_all_windows = true, close_scope = false, caption="Control GUI"| // params = [[synth, var, [range], label],...]
	var w, slider, p, a;
	close_all_windows.if({
		Window.allWindows.do{|w|
			((w.name != "internal server") && ((w.name != "Stethoscope") || close_scope)).if({w.close})
		};
	});
	w = Window(caption, Rect(300,200, 400, 40 * (params.size+1)));
	slider = Array.new(params.size);
	params.do{|p,i|
		slider.add(Slider(w,Rect(95, 10+(i*40), 300, 30)));
		a = StaticText(w, Rect(5, 10+(i*40), 85, 30));
		a.string = p[3];
		a.align = \right;
		slider[i].action = { |sl| (p[3]++": ").post; p[0].set(p[1], sl.value.linlin(0,1, p[2][0],p[2][1]).postln); };
		p[0].get(p[1], { |v| { slider[i].value = v.linlin(p[2][0],p[2][1], 0,1) }.defer } );
    };
    w.front();
};

"Basic functions & Engine sound initialized".postln
}.defer(0.3);
)

/// TESTING /////////////////////////////////////////////////////////////////////

~engine_out.ar[0]
~start_engine.value
~engine_loudness_out.scope
~engine_loudness_out.get
~stop_engine.value
~engine_out.scope

NetAddr.localAddr.sendMsg("/pitch_start", 0, 20)

~engine.set(\engineSpeed, 0.1)

///////////////////////////////////////////////////////

(
~engine_loudness.free;
	~engine_loudness = {
		var fft = FFT(~fft_buf, ~engine_out.ar);
	Out.kr(~engine_loudness_out, 0.05 * LPF.kr(Loudness.kr(fft),2));
	}.play(addAction:\addToTail);
)
~engine_loudness_smoothed_out = Bus.control(s);
(
~engine_loudness_smoothed.free;
~engine_loudness_smoothed = {
	Out.kr(~engine_loudness_smoothed_out, LPF.kr(~engine_loudness_out.kr, 1));
	//DelayL.kr(~engine_loudness_out.kr, 5, 5).scope;
	}.play(addAction:\addToTail);
//~engine_loudness_out.scope;
//~engine_loudness_smoothed_out.scope;
)
~engine_loudness_out.get;
~engine_loudness_smoothed_out.get
~engine_loudness_smoothed_out.scope

/////////////////////////////////////////////////////

s.freqscope

(
~control_gui2.value([
	[~x, \engineSpeed, [0,1], "engineSpeed"],
	[~x, \mixCylinders, [0,1], "mixCylinders"],
	[~x, \mixParabolic, [0,1], "mixParabolic"],
	[~x, \warpDelay, [0,1], "warpDelay"],
	[~x, \waveguideWarp, [0,1], "waveguideWarp"], //
	[~x, \wguideFeedback, [0,1], "wguideFeedback"], //
	[~x, \wguideLength1, [0,1], "wguideLength1"],
	[~x, \wguideLength2, [0,1], "wguideLength2"],
	[~x, \wguideWidth1, [0,1], "wguideWidth1"],
	[~x, \wguideWidth2, [0,1], "wguideWidth2"],
], false);
)

(200,210..10000).collect {|freq| (200/freq) ** 0.3333 };