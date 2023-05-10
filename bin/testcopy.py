#!/usr/bin/env python
# -*- coding:utf-8 -*-
from __future__ import print_function
import re
import glob
import shutil
import sys

class Move_So(object):

    def __init__(self):
        self.path = '/home/Landy/Desktop/mnt/hgfs/H5Server/bin/20200316/*'
        self.dir = []
        self.file_so = []
        self.dest = []


    def for_so_list(self):
        ls = glob.glob('*.so')
        return ls

    def dir_list(self):
        ls = glob.glob(self.path)
        # print(ls)
        return ls

    def nb(self):
        for dir in self.dir_list():
            dir = re.sub('\\\\','/',dir)
            dir_last = dir.split('/')[-1]
            # print(dir_last)

            for file in self.for_so_list():
                # libGame_tbnn_score_android.so
                files = file.split('.')[0].split('_')[1]
                if dir_last == files:
                    # print('%s  %s' % (file, dir))
                    try:
                        shutil.copy(file, dir)
                        #print(file,self.data_dir)
                    except IOError as e:
                        print("Unable to copy file. %s" % e)
                    except:
                        print("Unexpected error:", sys.exc_info())


cp_file = Move_So()
cp_file.nb()
