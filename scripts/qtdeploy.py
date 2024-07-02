#!/usr/bin/env python3

# The MIT License (MIT)

# Copyright (c) 2024 Fredrik Johansson

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

"""
This file is influenced by the macdeployqt tool in Qt.
This script is part of macdylibbundler and should be
platform independent, ie build Qt mac app bundles from linux.
This script has nothing preventing it from running on windows,
but parent process dylibbundler might still have some porting
issues to fix. as of 2024-06-30
"""

from common import question, print
from os import path, makedirs, listdir, environ, symlink
from collections import OrderedDict
import json, shutil, re, sys, os

class QmlFile():
  cache = OrderedDict()

  @staticmethod
  def addImport(importName:str, fromDir:str):
    """
    importName: this import name
    fromDir: file imported from this dir

    Add importName to cache.
    and resolves all this files possible imports
    """
    filePath = path.join(fromDir, importName)
    if filePath in QmlFile.cache:
      return

    def scanDir(rootPath, parts):
      if not parts:
        # must be a builtin import, ie: QtQuick.Controls ...etc
        QmlFile.cache[importName] = QmlFile(importName, True)
        #print(f"Builtin {importName}")
        return True

      part = parts.pop(0)
      qmlFile = p = path.join(rootPath, part)
      if not path.exists(p) and not path.isdir(p):
          qmlFile = f"{p}.qml"
      if path.isfile(qmlFile):
        QmlFile.cache[qmlFile] = QmlFile(qmlFile)
        return True
      elif path.isfile(f"{p}.js"):
        QmlFile.cache[f"{p}.js"] = QmlFile(f"{p}.js")
        return True
      elif path.isdir(p) and not parts:
        for e in [x for x in listdir(p) if x.endswith('.qml')]:
          QmlFile.addImport(f"{part}.{e[:-4]}", fromDir)
        return True
      return scanDir(p, parts)

    #print(f"scanning {importName}")
    if scanDir(fromDir, importName.split('.')):
      return

    print(f"*Failed to resolve qml import '{importName}' imported"
          f" from dir {fromDir}")
    exit(13)

  @staticmethod
  def postScanResolve(deployQt):
    "Run after scan is complete, looks up modules to copy"
    def sortAndFilter(dct, isModule):
      retDict = OrderedDict()
      dct = {k:v for k,v in dct.items() if v.isModule == isModule}
      keys = list(dct.keys())
      keys.sort(reverse=True)
      for key in keys:
        retDict[key] = dct[key]
      return retDict

    QmlFile.scriptCache = sortAndFilter(QmlFile.cache, False)
    QmlFile.moduleCache = sortAndFilter(QmlFile.cache, True)

    # find these modules in filesystem
    unresolved = []
    for key, mod in QmlFile.moduleCache.items():
      p = path.join(deployQt.qtDir, "qml", key.replace('.', path.sep))
      if path.exists(p):
        mod.modulePath = p
      else:
        for search in deployQt.settings['search_paths']:
          p = path.join(search, key)
          if path.exists(p):
            mod.modulePath = p
      if not mod.modulePath:
        print(f"*Failed to find qml module {key}, "
               "make sure it's included in app binary.")
        unresolved.append(key)

    for key in unresolved:
      del QmlFile.moduleCache[key]

  def __init__(self, filePath, isModule = False):
    self.imports = []
    self.filePath = filePath
    self.isModule = isModule
    print(f"new qml file:{filePath} module:{isModule}")
    if isModule:
      self.modulePath = ""
      return

    try:
      with open(filePath, "r") as file:
        self._readQmlFile(file)

    except IOError as e:
      print(f"*Failed to open {f} for reading")
      exit(1)

  def _readQmlFile(self, file):
    commentCnt = 0
    for line in file.readlines():
      found = re.match("^\\s*(?:import|internal)\\s+\"?([.\\w]+)\"?", line)
      if found != None:
        imp = found.group(1)
        self.imports.append(imp)
        QmlFile.addImport(imp, path.dirname(file.name))
      if re.match("/\\*", line):
        commentCnt += 1
      if re.match("\\*/", line):
        commentCnt -= 1
      if (commentCnt == 0 and
          (re.match("^\\s*pragma ", line) or
           re.match("~\\s*singleton ", line))
      ):
        continue
      if commentCnt < 1 and line == "":
        break

  def basename(self)->str:
    return path.basename(self.filePath)

  def dirname(self)->str:
    return path.dirname(self.filePath)

class DeployQt():
  dylibs = []
  def __init__(self):
    if len(sys.argv) != 2 or not path.exists(sys.argv[1]):
      self.printHelp()
    self.settings = json.loads(question("all_settings"))
    self.deps = json.loads(question("dylib_info"))
    print(json.dumps(self.settings, indent=2))
    print(json.dumps(self.deps, indent=2))

    self.hasQtLibs = False
    for arr in self.deps['src_files'].values():
      for dep in arr:
        if dep['framework_name'].startswith("Qt"):
          self.hasQtLibs = True

    if self.settings['create_app_bundle'] and self.hasQtLibs:
      self.init()
      self.createQtConf()
      self.deployPlugins()
      self.deployQmlImports()
      self.postProcess()

  def init(self)->None:
    if not self.tryFindQt():
      if not ('QT_PATH' in environ) or environ['QT_PATH'] == "":
        print("'No QT_PATH environment variable set, "+
              "ie: 'export QT_PATH=/path/to/your/Qt/Dir'")
        exit(2)

      self.qtDir = environ['QT_PATH']

    self.pluginSrcDir = path.join(self.qtDir, "plugins")
    self.pluginsDestDir = path.join(
      self.settings['app_bundle_contents_dir'], "PlugIns")
    self.qmlDestDir = path.join(
      self.settings['app_bundle_contents_dir'], "Resources",
      "qml", self.settings['app_bundle_name'])

    if 'QT_USE_DBG_LIBS' in environ:
      self.useDbgLibs = environ['QT_USE_DBG_LIBS'] == "true"
    else:
      self.log("Defaulting to not using debug libs. "+
               " to change: export QT_USE_DBG_LIBS=true")
    self.initQml()

  def tryFindQt(self)->False:
    def look(self, d):
      if path.isdir(d):
        entries = listdir(d)
        try:
          entries.index("Frameworks")
          entries.index("plugins")
          entries.index("qml")
          entries.index("lib")
          entries.index("modules")
          entries.index("translations")
        except ValueError:
          if path.basename(d) == "lib":
            return look(self, path.dirname(d))

        if path.exists(path.join(d, "lib", "QtCore.framework")):
          self.qtDir = d
          return True
      return False

    for d in self.settings['search_paths']:
      if look(self, d):
        return True
    return False

  def initQml(self)->None:
    if 'QML_DIRS' in environ and environ['QML_DIRS'] != "":
      self.qmlDirs = environ['QML_DIRS'].split(':')
    else:
      self.qmlDirs = []
      for fileToFix in self.deps['files_to_fix']:
        for d in ["./", "qml", "resources", "resources/qml"]:
          searchIn = path.realpath(
            path.join(path.dirname(fileToFix), d))
          if path.isdir(searchIn):
            for e in listdir(searchIn):
              if (e.endswith('.qml') and
                  path.isfile(path.join(searchIn, e))):
                self.qmlDirs.append(searchIn)
                break

  def printHelp(self) -> None:
    print("This script is not meant be be run directly.\n"
          "It is designed to run as a subprocess to dylibbundler\n"
          "Inputs to this script is set by environment variables\n"
          "\nie: in terminal: export VAR=... before running dylibbundler\n\n"
          "  QT_PATH=path/to/your/Qt/install/dir\n"
          "  QT_USE_DBG_LIBS=true\n"
          "  QML_DIRS=path/to/dir/containing/qml/file:other/dir/with/qml/files\n")
    exit(1)

  def checkDir(self, dirPath)->None:
    if not path.exists(dirPath):
      if (not self.settings['can_create_dir'] and
          not self.settings['create_app_bundle']):
        print(f"*{dirPath} does not exist, not allowed to create."+
              " Change input switches to allow ie: --overwrite-dir")
        exit(2)
      makedirs(dirPath)

  def checkFile(self, filePath)->None:
    if (path.exists(filePath) and
        not self.settings['can_overwrite_files']
    ):
      print(f"*{filePath} does already exists, can't overwrite."+
            " Change input switches top allow ie: --overwrite-files")
      exit(2)

  def log(self, msg)->None:
    if self.settings['verbose']:
      print(msg)

  def copyFile(self, src:str, dst:str, check = True)->bool:
    if check and path.exists(dst) and not self.settings['can_overwrite_files']:
      print(f"*{dst} already exists, and --overwrite-files is false")
      exit(13)
    if check and not path.exists(src):
      print(f"*{src} does not exist, can't copy file to {dst}")
      exit(13)

    try:
      shutil.copy(src, dst)
      if dst.endswith(".dylib") and not(dst in DeployQt.dylibs):
        DeployQt.dylibs.append(dst)
      self.log(f"copied:{src} \nto:     {dst}")
      return True
    except:
      print(f"/!\\copy failed\nfrom:{src} \nto  {dst}")
      return False

  def postProcess(self)->bool:
    cmd = {"add_search_paths": self.searchPaths()}
    res = json.loads(question(json.dumps(cmd)))
    if "error" in res and res['error']:
      print("Failed to set search paths to parent process, error "
            f"{res['error']}")
      return False

    cmd = {"fixup_binaries": DeployQt.dylibs}
    res = json.loads(question(json.dumps(cmd)))
    if "error" in res and res['error']:
      print(f"*Failed to run strip on {dst}, error:{res['error']}")
      return False
    return True

  def createQtConf(self)->None:
    qtConfDir = path.join(
      f"{self.settings['app_bundle_contents_dir']}",
      "Resources")

    self.checkDir(qtConfDir)
    qtConfDir += "qt.conf"
    self.checkFile(qtConfDir)


    self.log("Creating qt.conf")
    with open(qtConfDir, "w") as file:
      file.write("[Paths]\nPlugins\ imports = imports\n")

  def searchPaths(self)->None:
    paths = []
    for plug in self.pluginsToDeploy:
      d = path.join(self.pluginsDestDir, path.dirname(plug))
      if not (d in paths):
        paths.append(d)
    return paths


  def deployPlugins(self)->None:
    self.log(f"Deploying plugins from:{self.pluginSrcDir}")

    self.pluginsToDeploy = []
    self.addPlugins("platforms")
    self.addPlugins("printsupport", fail=False)
    self.addPlugins("styles")
    self.addPlugins("iconengines")

    self.scanForPlugins()

    for plugin in self.pluginsToDeploy:
      srcPath = path.join(self.pluginSrcDir, plugin)
      dstPath = path.join(self.pluginsDestDir, plugin)
      self.log(f"Copy: {srcPath} to {dstPath}")

      if srcPath.endswith(".dylib"):
          dbgPth = srcPath.replace(".dylib", "_debug.dylib")
          if path.exists(dbgPth):
            srcPath = dbgPth

      self.checkDir(path.dirname(dstPath))
      self.copyFile(srcPath, dstPath)

  def addPlugin(self, plugdir:str, plugname:str, fail=True)->None:
    plug = path.join(self.pluginSrcDir, plugdir, plugname)
    if path.exists(plug):
      self.pluginsToDeploy.append(path.join(plugdir, plugname))
    else:
      print(f"*{plug} not found")
      if fail: exit(13)

  def getLibInfix(self):
    for d in listdir(path.join(self.qtDir, "lib")):
      if (d.startswith('QtCore') and
          d.endswith('.framework') and
          not ('5Compat' in d)):
          return d[6:len(d)-10] # return the  stuff between

  def scanForPlugins(self)->None:
    qt = f"Qt{self.getLibInfix()}"
    usesSvg = False
    for file, dependencies in self.deps['src_files'].items():
      for dep in dependencies:
        if dep['is_framework']:
          fw = dep['framework_name']
          if fw == f"{qt}Svg":
            usesSvg = True
          elif fw == f"{qt}Network":
            self.addPlugins("tls")
            self.addPlugins("networkinformation")
          elif fw == f"{qt}Gui":
            def kbd(lib):
              if lib.startswith('libqtvirtualkeyboard'):
                self.addPlugins('virtualkeyboard', fail=False)
              return True
            self.addPlugins("platforminputcontexts", predicate=kbd)
          elif fw == f"{qt}Sql":
            self.addPlugins("sqldrivers")
          elif fw == f"{qt}WebVieW":
            self.addPlugins("webview")
          elif fw == f"{qt}Multimedia":
            self.addPlugins("multimedia")
          elif fw == f"{qt}3DRender":
            self.addPlugins("sceneparsers")
            self.addPlugins("geometryloaders")
            self.addPlugins("renderers")
          elif fw == f"{qt}3DQuickRender":
            self.addPlugins("renderplugins")
          elif fw == f"{qt}Positioning":
            self.addPlugins("position")
          elif fw == f"{qt}Location":
            self.addPlugins("geoservices")
          elif fw == f"{qt}TextToSpeech":
            self.addPlugins("texttospeech")
          elif fw == f"{qt}SerialBus":
            self.addPlugins("canbus")

    def img(lib):
      if usesSvg and 'qsvg' in lib:
        return False
      return True
    self.addPlugins("imageformats", predicate=img)

  def addPlugins(self, directory:str, **kwargs)->None:
    "kwargs: fail = True, predicate=func(plugName)"
    pred = kwargs['predicate'] if 'predicate' in kwargs else lambda a: True
    fail = kwargs['fail'] if 'fail' in kwargs else True
    plugDir = path.join(self.pluginSrcDir, directory)
    if not path.exists(plugDir):
      if not fail: return
      print(f"*{plugDir} is not a valid plugin dir")
      exit(13)

    for filename in listdir(plugDir):
      f = path.join(self.pluginSrcDir, directory, filename)
      if path.isfile(f) and f.endswith(".dylib"):
        if not f.endswith("_debug.dylib"):
          if not (f in self.pluginsToDeploy) and pred(filename):
            self.addPlugin(directory, filename, fail)
            #print(f"found {self.pluginsToDeploy[-1]}")

  def scanQmlImports(self):
    for directory in self.qmlDirs:
      for filename in listdir(directory):
        f = path.join(directory, filename)
        if f.endswith(".qml") and path.isfile(f):
          QmlFile.addImport(
            path.basename(f)[:-4], path.dirname(f))
    QmlFile.postScanResolve(self)

  def deployQmlImports(self):
    self.scanQmlImports()

    for key, inst in QmlFile.moduleCache.items():
      self.deployQmlModule(key, inst)

  def deployQmlModule(self, name, qmlInst):
    self.log(f"Copy {name} module")

    # fd for relative paths on symlinks
    contDir = os.open(self.settings['app_bundle_contents_dir'], os.O_RDONLY)

    def deployDir(srcDir, destDir):
      for entry in listdir(srcDir):
        if path.isdir(path.join(srcDir, entry)):
          deployDir(path.join(srcDir, entry), path.join(destDir, entry))
          continue
        print(name, entry)
        src = path.join(srcDir, entry)
        to = path.join(path.join(self.qmlDestDir, destDir), entry)
        if entry.endswith('.dylib'):
          if entry.endswith("_debug.dylib"):
            continue
          # can't store .dylibs in Resources dir
          # Store them in Contents/PlugIns/quick and then symlink
          binDir = path.join("PlugIns", "quick", destDir)
          linkDir = path.join("Resources", "quick", destDir)
          binDirPath = path.join(
            self.settings['app_bundle_contents_dir'], binDir)
          linkDirPath = path.join(
            self.settings['app_bundle_contents_dir'], linkDir)
          dst = path.join(linkDir, entry)
          if not path.exists(binDirPath):
            makedirs(binDirPath)
          if not path.exists(linkDirPath):
            makedirs(linkDirPath)

          self.copyFile(src, path.join(binDirPath, entry), False)
          if not path.exists(path.join(linkDirPath, entry)):
            src = path.relpath(path.join(binDir, entry), linkDir)
            #print(f"\nsrc:    {src} \ndst:    {dst}")
            symlink(src, dst, dir_fd=contDir)
        else:
          if not path.exists(path.dirname(to)):
            makedirs(path.dirname(to))
          self.copyFile(src, to, False)

    print("module", qmlInst.modulePath)

    deployDir(qmlInst.modulePath, path.basename(qmlInst.modulePath))

if __name__ == "__main__":
  DeployQt()
