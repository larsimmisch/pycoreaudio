#!/usr/bin/env python

from distutils.core import setup, Extension

setup(name="coreaudio", version="0.1",
      ext_modules=[
         Extension("coreaudio", ["coreaudio.c"],
                   extra_link_args=["-framework", "CoreAudio",
                                    "-framework", "AudioUnit"])
         ]
)

