#include "../include/globals.h"

void InitAudio();
double GetAudioRow();

HSTREAM stream;
sync_device *rocket;

SyncTracks g_syncTracks;

static const float bpm = 140.0f; /* beats per minute */
static const int rpb = 8; /* rows per beat */
static const double row_rate = (double(bpm) / 60) * (double)rpb;

// T‰‰ on t‰‰ll‰ laiskuuden vuoksi
void UpdateTitle(double currentTime, QWORD currentPos) {
	static int	frame = 0;
	static char	title[128];
	static int  fps=0;
	static double to=0.0;
	static double lastTime = 0;

	if (g_options.fullscreen || currentTime <= 0) {
		return;
	}
	if (currentTime > lastTime && (currentTime - lastTime) < 0.001) {
		return;
	}

	frame++;

	if( (currentTime-to)>0.25f || currentTime < to)
	{
		fps = frame*4;
		to = currentTime;
		frame = 0;
	}
	//fps = t;
	sprintf(title, "time: %f fps: %d pos: %I64d", currentTime, fps, currentPos);
	SetWindowText(g_hWnd, title);
	lastTime = currentTime;
}
double currentFrame = 0;
static double bass_get_row(HSTREAM h)
{
	if(g_options.saveImages) {
		currentFrame += 1.0/g_options.saveFrameRate;
		return currentFrame * row_rate;
	}
	
	QWORD pos = BASS_ChannelGetPosition(h, BASS_POS_BYTE);
	double time = BASS_ChannelBytes2Seconds(h, pos);
//#ifndef SYNC_PLAYER
	UpdateTitle(time, pos);
//#endif
	return time * row_rate;
}

#ifndef SYNC_PLAYER


static void bass_pause(void *d, int flag)
{
	HSTREAM h = *((HSTREAM *)d);
	if (flag)
		BASS_ChannelPause(h);
	else
		BASS_ChannelPlay(h, false);
}

static void bass_set_row(void *d, int row)
{
	HSTREAM h = *((HSTREAM *)d);
	QWORD pos = BASS_ChannelSeconds2Bytes(h, row / row_rate);
	BASS_ChannelSetPosition(h	, pos, BASS_POS_BYTE);
}

static int bass_is_playing(void *d)
{
	HSTREAM h = *((HSTREAM *)d);
	return BASS_ChannelIsActive(h) == BASS_ACTIVE_PLAYING;
}

static struct sync_cb bass_cb = {
	bass_pause,
	bass_set_row,
	bass_is_playing
};

#endif /* !defined(SYNC_PLAYER) */

void GetTracks() 
{
	/* get tracks */
	g_syncTracks.cam = sync_get_track(rocket, "cam");
	g_syncTracks.camTime = sync_get_track(rocket, "camTime");

	g_syncTracks.camRotX = sync_get_track(rocket, "camRotX");
	g_syncTracks.camRotY = sync_get_track(rocket, "camRotY"); 
	g_syncTracks.camRotZ = sync_get_track(rocket, "camRotZ");

	g_syncTracks.camFov = sync_get_track(rocket, "camFov");

	g_syncTracks.fade = sync_get_track(rocket, "fade");
	g_syncTracks.saturation = sync_get_track(rocket, "saturation");

	g_syncTracks.synk1 = sync_get_track(rocket, "synk1");
	g_syncTracks.synk2 = sync_get_track(rocket, "synk2");
	g_syncTracks.synk3 = sync_get_track(rocket, "synk3");
	g_syncTracks.synk4 = sync_get_track(rocket, "synk4");
	g_syncTracks.synk5 = sync_get_track(rocket, "synk5");
	g_syncTracks.synk6 = sync_get_track(rocket, "synk6");
	g_syncTracks.synk7 = sync_get_track(rocket, "synk7");
	g_syncTracks.synk8 = sync_get_track(rocket, "synk8");
	g_syncTracks.synk9 = sync_get_track(rocket, "synk9");

	g_syncTracks.scene = sync_get_track(rocket, "scene");
	g_syncTracks.blur = sync_get_track(rocket, "blur");
	g_syncTracks.blurPower = sync_get_track(rocket, "blurPower");
	g_syncTracks.bloom = sync_get_track(rocket, "bloom");
	g_syncTracks.bloomPower = sync_get_track(rocket, "bloomPower");

	g_syncTracks.radial = sync_get_track(rocket, "radial");
	g_syncTracks.noise = sync_get_track(rocket, "noise");
	g_syncTracks.image = sync_get_track(rocket, "image");
	g_syncTracks.imageFade = sync_get_track(rocket, "imageFade");

	g_syncTracks.part1 = sync_get_track(rocket, "part1");
	g_syncTracks.part2 = sync_get_track(rocket, "part2");
	g_syncTracks.part3 = sync_get_track(rocket, "part3");
	g_syncTracks.part4 = sync_get_track(rocket, "part4");

}

void PlayMusic() 
{
	if(!g_options.saveImages) {
		BASS_Start();
		BASS_ChannelPlay(stream, false);
	}
}


void InitAudio() {
	/* init BASS */
	if (!BASS_Init(-1, 44100, 0, 0, 0)) {
		Quit("Failed to initialize bass");

	}

	stream = BASS_StreamCreateFile(false, "data/demo.mp3", 0, 0,
		BASS_STREAM_PRESCAN);
	if (!stream) {
		Quit("failed to open tune");
	}

	rocket = sync_create_device("data/sync/s");
	if (!rocket) {
		Quit("out of memory?");
	}

#ifndef SYNC_PLAYER
	if (sync_connect(rocket, "localhost", SYNC_DEFAULT_PORT))
		Quit("failed to connect to host");
#endif

	GetTracks();
}

double GetAudioRow() {
	double row = bass_get_row(stream);
	
	//BASS_ChannelSetAttribute(
	//	stream,
	//	BASS_ATTRIB_VOL,
	//	(float)sync_get_val(g_syncTracks.synk1, row)
	//	);

#ifndef SYNC_PLAYER
	if (sync_update(rocket, (int)floor(row), &bass_cb, (void *)&stream)) {
		sync_connect(rocket, "localhost", SYNC_DEFAULT_PORT);
	} 

#endif

	return row;
}