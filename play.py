#!/usr/bin/env python3

import coreaudio
import threading
from optparse import OptionParser
import wave

def fourcctoi(v):
    # empty string is zero
    if not v:
        return 0
    # otherwise, the length must be 4
    if len(v) != 4:
        raise ValueError('FOURCC string must be four characters long')

    return (ord(v[0]) << 24) + (ord(v[1]) << 16) + (ord(v[2]) << 8) + ord(v[3])

def au_wav_prepare(au, fn, verbose = False):
    """Open a wav file called 'fn' and set the stream format based upon the
    information in the wav header.

    Returns the open file."""

    f = wave.open(fn, 'r')

    if verbose:
        print("""%s:
    sampling rate: %d
    channels: %d
    sample width: %d
    """ % (fn, f.getframerate(), f.getnchannels(), f.getsampwidth()))

    sd = coreaudio.AudioStreamBasicDescription(
        f.getframerate(),
        coreaudio.kAudioFormatLinearPCM,
        coreaudio.kAudioFormatFlagIsSignedInteger | \
        coreaudio.kAudioFormatFlagsNativeEndian | \
        coreaudio.kAudioFormatFlagIsNonInterleaved,
        f.getsampwidth() * f.getnchannels(),
        f.getnchannels(), f.getsampwidth(), 1, f.getsampwidth() * 8)

    au.SetStreamFormat(sd)

    return f

def play(au, f):
    """Play a single file on 'au'. 'f' must be an open file."""

    def render_callback(flags, time, bus, frames, nbuffers, user_data):
        done.acquire()
        buf = f.readframes(frames)
        # print(f'read {len(buf)} bytes')

        if not buf:
            done.notify()
            done.release()
            # This will implicitly stop the playback without a warning
            return (None, b'')

        done.release()

        # pad the last frame with silence
        sil = frames * 2 - len(buf)
        if sil:
            buf = buf + b'\0' * sil
        return (None, buf)

    done = threading.Condition()
    done.acquire()

    print('Setting render callback')
    au.SetRenderCallback(render_callback)
    print('Starting')
    au.Start()

    done.wait()
    au.SetRenderCallback(None)

def open_default_au(manufacturer = 'appl'):

    desc = coreaudio.AudioComponentDescription(
        coreaudio.kAudioUnitType_Output,
        coreaudio.kAudioUnitSubType_DefaultOutput, fourcctoi(manufacturer))
    print(desc)

    c = coreaudio.AudioComponentFindNext(None, desc)
    au = coreaudio.AudioComponentInstanceNew(c)

    au.Initialize()

    return au

if __name__ == '__main__':
    # print fourcctoi('appl')
    # print coreaudio.kAudioUnitManufacturer_Apple

    parser = OptionParser(usage='usage: %prog [options] <file>')
    parser.add_option("-m", "--manufacturer", dest="manufacturer",
                      help="Open the Audio Unit from 'manufacturer'. ",
                      default = 'appl')
    parser.add_option("-v", "--verbose", dest="verbose",
                      action = "store_true",
                      help="Print more logging information'. ",
                      default = True)

    options, args = parser.parse_args()
    if len(args) < 1:
        parser.error('need at least one file argument')

    au = open_default_au(options.manufacturer)

    for a in args:
        print('playing %s' % a)
        f = au_wav_prepare(au, a, options.verbose)
        play(au, f)
        f.close()
