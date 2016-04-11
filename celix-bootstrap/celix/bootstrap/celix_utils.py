# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

import cmd
import glob
import os.path
import re
import readline
import sys

class PathCompleter(object):

    def __init__(self, suffix=None):
        self.suffix=suffix

    def _listdir(self, root=''):
        res = []
        for name in glob.glob(root + '*'): 
            path = os.path.join(root, name)
            if os.path.isdir(path):
                name += os.sep
            	res.append(name)
	    else:
		if self.suffix is None:
            		res.append(name)
		elif name.endswith(self.suffix):
            		res.append(name)

        return res

    def complete(self, text, state):
        buffer = readline.get_line_buffer()
        return self._listdir(buffer.strip())[state]


class FileContentCompleter(object):
    def __init__(self, filename, keywords=[]):
        self._indexFile(filename, keywords)

    def _indexFile(self, filename, keywords=[], delimiter_chars=":;#,.!?{}*)(=\\\"/"):
        try:
	    txt_fil = open(filename, "r")
	    self.found_words = []

            for line in txt_fil:
		# ignore commented lines
		if not (line.strip()).startswith(("/*", "*", "//")):
                    words = line.split()
                    words2 = [ word.strip(delimiter_chars) for word in words ]

                    for word in words2:
                        if not word in self.found_words and not word in keywords and not word.isdigit() and not any(i in word for i in delimiter_chars):
                            self.found_words.append(word)

            txt_fil.close()
        except IOError as ioe:
             sys.stderr.write("Caught IOError: " + repr(ioe) + "\n")
        except Exception as e:
             sys.stderr.write("Caught Exception: " + repr(e) + "\n")

    def checkWord(self, word):
        if word in self.found_words:
		return True
	else:
		return False

    def complete(self, text, state):
        buffr = readline.get_line_buffer()
	wordlist = [w for w in self.found_words if w.startswith(buffr)]

        return wordlist[state]

def checkInput(msg, regex='', default=''):
	while True:
		msgDefault = msg + ' [' + default + ']: ' if default is not '' else msg + ': '
 		inpt = raw_input(msgDefault).strip() or default
		
		if regex is not '':
			res = re.match(regex, inpt);
			if res is not None and inpt == res.group():
				return inpt
			else:
				print('Invalid Input.')
		else:
			return inpt

def yn(msg, default = 'y'):
	while True:
		addComponent = raw_input("%s [%s]:" % (msg, default)) or default
      		if addComponent.lower() in ('y', 'yes', 'n', 'no'):
			return addComponent.lower() in ('y', 'yes')
		else:
			print("Invalid Input.")


def checkCelixInstallation():
	while True:
		comp = PathCompleter()
		# we want to treat '/' as part of a word, so override the delimiters
		readline.set_completer_delims(' \t\n;')
		readline.parse_and_bind("tab: complete")
		readline.set_completer(comp.complete)

 		installDir = checkInput('Please enter celix installation location', '(?:/[^/]+)*$', '/usr/local')

		if os.path.exists(installDir + '/bin/celix'):
			return installDir
		else:
			print('celix launcher could not be found at ' + installDir + '/bin/celix')



def checkInclude(msg, regex):
	comp = PathCompleter(".h")
	# we want to treat '/' as part of a word, so override the delimiters
	readline.set_completer_delims(' \t\n;')
	readline.parse_and_bind("tab: complete")
	readline.set_completer(comp.complete)

	headerFile = checkInput(msg, regex)

	return headerFile

def checkIncludeContent(msg, filename):
	keywords = ["auto","break","case","char","const","continue","define", "default","do","double","else","endif", "enum","extern","float","for","goto","if","ifndef", "include", "inline","int","long","register","restrict","return","short","signed","sizeof","static","struct","switch","typedef","union","unsigned","void","volatile","while", "celix_status_t"];

	comp = None	

	if os.path.exists(filename):
		comp = FileContentCompleter(filename, keywords)
		readline.set_completer_delims(' \t\n;')
		readline.parse_and_bind("tab: complete")
		readline.set_completer(comp.complete)

	inpt = checkInput(msg, '[^\s]+');

	if comp is not None:
		while(comp.checkWord(inpt) == False and yn('\''+ inpt + '\'  was not found in the given include file. Do you really want to use it?', 'n') == False):
			inpt = checkInput(msg, '[^\s]+');

	return inpt
