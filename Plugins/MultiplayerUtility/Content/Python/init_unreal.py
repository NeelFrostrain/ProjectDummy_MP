# Copyright Epic Games, Inc. All Rights Reserved.
"""
Multiplayer Utility - Unreal Python Initialization Hook
Runs automatically on Unreal Editor launch.
"""
import sys
import os

try:
    import unreal
    project_dir = unreal.Paths.project_dir()
    scripts_dir = os.path.normpath(os.path.join(project_dir, "Plugins", "MultiplayerUtility", "Scripts"))
    content_py_dir = os.path.normpath(os.path.join(project_dir, "Content", "Python"))
    plugin_py_dir = os.path.normpath(os.path.join(project_dir, "Plugins", "MultiplayerUtility", "Content", "Python"))

    for p in (scripts_dir, content_py_dir, plugin_py_dir):
        if os.path.exists(p) and p not in sys.path:
            sys.path.append(p)

    unreal.log("[MultiplayerUtility] Python utilities loaded successfully.")
except Exception as e:
    pass

