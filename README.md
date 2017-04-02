# TagSupporter
GUI application to ease the task of (properly) tagging audio files in a directory.

## Why another audio file tagging software?
Many MP3 files you get form different sources (e.g. when bying MP3s on Amazon) usually do not pay too much attention on using the proper genre, even if they are otherwise well tagged.
In addition, many digital downloads change the album title, so that grouping files of the same album becomes impossible for collection databases such as Amarok or Kodi.

So I often found myself cleaning up the tags of my audio files and looking up the information for that from various internet sources.
Not all sources have the same quality. I for example, tend to trust Wikipedia a lot more than Discogs and would prepfer both over google.
Hence, I usually tried to find the respective album or single on the most trusted source first and then graudally widened my search.
Needless to say that this is a rather time consuming and tedious task.

So the GUI application in this repository is specifically tailored to ease this process and help collect the relevant information from several online sources.
It furthermore attempts to open the local amarok collection database and show already existing genres, artists, albums and tracks with the same or similar names to avoid spelling variations.

## Requirements:
- Qt 5.4
- libtag 1.11
- libmysqld (for openeing and browsing the Amarok Database)
