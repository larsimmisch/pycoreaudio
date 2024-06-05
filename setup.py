#!/usr/bin/env python3

from setuptools import Extension, setup

setup(name="coreaudio", version="0.1",
   ext_modules=[
      Extension("coreaudio", ["coreaudio.c"],
         extra_link_args=[
            "-framework", "CoreAudio",
            "-framework", "AudioUnit"
         ]
      )
   ]
)
