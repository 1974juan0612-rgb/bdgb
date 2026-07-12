"""Asistente de calibracion para NotebookLM via PyAutoGUI.
Ejecutar: python calibrar.py
Abre NotebookLM primero con el acceso directo del escritorio."""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gui_utils import calibrate

if __name__ == "__main__":
    calibrate()
