#include <iostream>
#include "VoiceRecognition.hpp"
#undef max
#undef min
#include "Event/Queue.hpp"
#include <unistd.h>
#include "LocalPath.hpp"
#include <windef.h> // for MAX_PATH
#include <string.h>
#include "Screen/Key.h"
#include "Interface.hpp"
#include "Audio/raw_play.hpp"

#ifdef KOBO
#define TOPHAT_CONFIDENCE 0.5
#elif defined(ANDROID)
#define TOPHAT_CONFIDENCE 0.5
#else
#define TOPHAT_CONFIDENCE 0.5
#endif
#define COMMAND_CONFIDENCE 0.5

#define KEYWORD_KEYCODE_SIZE 503 /* the prime number bigger than SDLK_LAST */

#define SKIP_PRELUDE 6000
#define NOISE_LEVEL 300

double tophat_confidence = TOPHAT_CONFIDENCE;
double command_confidence = COMMAND_CONFIDENCE;

EventQueue *static_queue; /* to connect class and static function */

static unsigned short keyword_keycode[KEYWORD_KEYCODE_SIZE];
static struct {
  char const *keyword;
  unsigned short keycode;
} keyword_keycode_table[] = {
  {"0", '0'},
  {"1", '1'},
  {"2", '2'},
  {"3", '3'},
  {"4", '4'},
  {"5", '5'},
  {"6", '6'},
  {"7", '7'},
  {"8", '8'},
#if 0 /* segmentation fault */
  {"9", '9'},
#endif
  {"ESCAPE", KEY_ESCAPE},
  {"Quick", KEY_F1},
  {"Analysis", KEY_F2},
  {"Checklist", KEY_F3},
  {"Traffic", KEY_F4},
  {"Setup", KEY_F6},
  {"Task", KEY_F7},
  {"Menu", KEY_MENU},
  {"Navigation", KEY_APP1},
  {"Display", KEY_APP2},
  {"Configure", KEY_APP3},
  {"Info", KEY_APP4},
  {"minus", KEY_DOWN},
  {"Previous", 'P'},
  {"Circling", 'C'},
  {"Rotate", '8'},
  {"Target", 'T'},
  {"DOWN", KEY_DOWN},
  {"Exit", '0'},
  {"next", 'N'},
  {"FINISH", 'F'},
  {"FINAL", 'F'},
  {"quit", '0'},
  {"START", 'S'},
#if 0 /* commented out in tophat.voca */
  {"RETURN", KEY_RETURN},
  {"UP",  KEY_UP},
  {"RIGHT", KEY_RIGHT},
  {"LEFT", KEY_LEFT},
  {"plus", SDLK_UP},
  {"back", SDLK_b},
  {"Calculator", '7'},
  {"Zoom", KEY_UP},
#endif
};

static unsigned short
keyword_hash(char const *keyword)
{
  unsigned int hash = 0;
  char const *p;

  for (p = keyword; *p; )
    hash =  (hash << 5) ^ (hash >> 2) ^ *p++;
  return hash % KEYWORD_KEYCODE_SIZE;
}

/*
 * convert recogniszed word to keycode.
 */
static unsigned KeywordSearch(const char *word)
{
  return keyword_keycode[keyword_hash(word)];
}

static void
PushWords(const char *word)
{
  static_queue->PushKeyPress(KeywordSearch(word));
}

/* pass from record_speechlen() to monitor_repeat()  */
static int speechlen = 0;

/** 
 * Callback to output final recognition result.
 * This function will be called just after recognition of an input ends
 * 
 */
static void
record_speechlen(Recog *recog, void *dummy)
{
  ADIn *adin = recog->adin;
  if (!adin)
    return;
  if (adin->speechlen > speechlen)
    speechlen = adin->speechlen;
}

static void
monitor_repeat(Recog *recog)
{
  ADIn *adin = recog->adin;
  if (!adin)
    return;
  SP16 *speech = adin->speech;
  if (!speech)
    return;
  if (speechlen <= 0)
    return;

  /* skip slient portion to make shorter playing time */
  for (int skip = 0; abs(*speech) < NOISE_LEVEL; speech++, speechlen--)
    if (++skip > SKIP_PRELUDE)
      break;
  RawPlayback *raw_playback = new RawPlayback();
  raw_playback->playback_mem(speech, speechlen);
  delete raw_playback;

  speechlen = 0; /* clear for next voice */
}

/**
 * Callback to output final recognition result.
 * This function will be called just after recognition of an input ends
 *
 */
static void
output_result(Recog *recog, void *dummy)
{
  int i;
  WORD_INFO *winfo;
  WORD_ID *seq;
  int seqnum;
  int n;
  Sentence *s;
  RecogProcess *r;

  /* all recognition results are stored at each recognition process
     instance */
  for(r=recog->process_list;r;r=r->next) {

    /* skip the process if the process is not alive */
    if (! r->live)
      continue;

    /* result are in r->result.  See recog.h for details */

    /* check result status */
    if (r->result.status < 0)      /* no results obtained */
      continue;

    /* output results for all the obtained sentences */
    winfo = r->lm->winfo;

    for(n = 0; n < r->result.sentnum; n++) { /* for all sentences */

      s = &(r->result.sent[n]);
      seq = s->word;
      seqnum = s->word_num;

      const VoiceRecogSettings &voice =
	CommonInterface::GetComputerSettings().voice_recog;
      if (voice.repeat_voice)
	monitor_repeat(recog); /* repeat */
      /* output word sequence like Julius */
      for (i = 1; i < seqnum - 1; i++)
	printf("%s ", winfo->woutput[seq[i]]);
      for (i = 1; i < seqnum - 1; i++)
	printf("%5.3f ",  s->confidence[i]);
      printf("\n");
      /* flush output buffer */
      fflush(stdout);
      if (s->confidence[1] > tophat_confidence && /* first word */
	  strcmp(winfo->woutput[seq[1]], "tophat") == 0 &&
	  s->confidence[2] > command_confidence) { /* second word */
	PushWords(winfo->woutput[seq[2]]);
      }
    }
  }
}

VoiceRecognition::VoiceRecognition(EventQueue &_queue):
  Thread("VoiceRecognition"), queue(_queue)
{
  unsigned int ti;
  unsigned short x;

  memset(keyword_keycode, 0, KEYWORD_KEYCODE_SIZE);
  for (ti = 0;
       ti < sizeof(keyword_keycode_table)/sizeof(keyword_keycode_table[0]);
       ti++) {
    x = keyword_hash(keyword_keycode_table[ti].keyword);
    if (keyword_keycode[x] != 0) {
      printf("same keyword_keycode: %s\n", keyword_keycode_table[ti].keyword);
      exit(99);
    }
    keyword_keycode[x] = keyword_keycode_table[ti].keycode;
  }
}

VoiceRecognition::~VoiceRecognition()
{
}

bool
VoiceRecognition::Start()
{
  assert(!IsDefined());

  return Thread::Start();
}

void
VoiceRecognition::Stop()
{
  /* wait for the thread to finish */
  Join();
}

static char const *argv[] = {
  "",
  "-C",
  NULL,
  "-input",
  "mic",
  "-quiet"
};
static int argc = sizeof(argv) / sizeof(*argv);

void
VoiceRecognition::Run()
{
  int ret;
  static TCHAR path[MAX_PATH];

  static_queue = &queue;

  LocalPath(path, _T("tophat.log"));
  FILE *file = fopen(path, "a");
  if (file == NULL) {
    fprintf(stderr, "can't open: %s.\n", path);
    return;
  }
  jlog_set_output(file);

  LocalPath(path, _T("julius/tophat.jconf"));
  argv[2] = (char *)path;

  /************/
  /* Start up */
  /************/
  /* 1. load configurations from command arguments */
  jconf = j_config_load_args_new(argc, const_cast<char **>(argv));
  /* else, you can load configurations from a jconf file */
  //jconf = j_config_load_file_new(jconf_filename);
  if (jconf == NULL) {		/* error */
    fprintf(stderr, "Try `-help' for more information.\n");
    return;
  }
  
  /* 2. create recognition instance according to the jconf */
  /* it loads models, setup final parameters, build lexicon
     and set up work area for recognition */
  recog = j_create_instance_from_jconf(jconf);
  if (recog == NULL) {
    fprintf(stderr, "Error in startup\n");
    return;
  }

  /*********************/
  /* Register callback */
  /*********************/
  /* register result callback functions */
  callback_add(recog, CALLBACK_ADIN_TRIGGERED, record_speechlen, NULL);
  callback_add(recog, CALLBACK_RESULT, output_result, NULL);

  /**************************/
  /* Initialize audio input */
  /**************************/
  /* initialize audio input device */
  /* ad-in thread starts at this time for microphone */
  if (j_adin_init(recog) == FALSE) {    /* error */
    return;
  }

  /***********************************/
  /* Open input stream and recognize */
  /***********************************/
  /* raw speech input (microphone etc.) */
  switch(j_open_stream(recog, NULL)) {
  case 0:			/* succeeded */
    break;
  case -1:      		/* error */
    fprintf(stderr, "error in input stream\n");
    return;
  case -2:			/* end of recognition process */
    fprintf(stderr, "failed to begin input stream\n");
    return;
  }

  /* by default, all messages will be output to standard out */
  /* to disable output, uncomment below */
  jlog_set_output(NULL);
  fclose(file);

  /**********************/
  /* Recognization Loop */
  /**********************/
  /* enter main loop to recognize the input stream */
  /* finish after whole input has been processed and input reaches end */
  ret = j_recognize_stream(recog);
  if (ret == -1) return;	/* error */

  /* calling j_close_stream(recog) at any time will terminate
     recognition and exit j_recognize_stream() */
  j_close_stream(recog);

  j_recog_free(recog);
}
