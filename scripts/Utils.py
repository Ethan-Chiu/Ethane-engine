import time
import os
import subprocess
import urllib.request
from tqdm import tqdm
import shutil
from io import BytesIO
from pathlib import Path

import zipfile
import tarfile
from colorama import init, Fore, Back, Style

def _listen_data_stream_from_url(url: str, callback):
    with urllib.request.urlopen(url) as response:
        total_size = int(response.headers.get('content-length'))
        block_size = 1024
        downloaded_size = 0
        start_time = time.time()
        with tqdm(total=total_size, unit='B', unit_scale=True) as progress_bar:
            while True:
                buffer = response.read(block_size)
                if not buffer:
                    break
                callback(buffer)
                downloaded_size += len(buffer)
                # show progress
                progress_bar.update(len(buffer))
                elapsed_time = time.time() - start_time
                unit = "KB"
                download_speed = (downloaded_size/1024) / elapsed_time
                if (download_speed > 1024):
                    unit = "MB"
                    download_speed = download_speed / 1024
                progress_bar.set_postfix(download_speed=f"{download_speed:.2f} {unit}/s")


def download_file(url, destination):
    os.makedirs(os.path.dirname(destination), exist_ok=True)
    with open(destination, 'wb') as file:
        # Download the file
        print("Downloading file...")
        
        write_to_file = lambda data: file.write(data)
        _listen_data_stream_from_url(url, write_to_file)

    print("Download completed.")


def download_archive(url, destination_dir):
    os.makedirs(os.path.dirname(destination_dir), exist_ok=True)

    buffer = BytesIO()
    # Download the file
    print("Downloading...")

    write_to_buffer = lambda data: buffer.write(data)
    _listen_data_stream_from_url(url, write_to_buffer)

    print("Download completed. Start extracting...")

    filename = url.split("/")[-1]
    if 'tar' in filename:
        with tarfile.open(fileobj=buffer) as tfile:
            tfile.extractall(destination_dir)
    else:
        with zipfile.ZipFile(buffer) as zfile:
            zfile.extractall(destination_dir)
    print("extraction completed")


def filename_exists_in_dir(filename:str, dir_path:str):
    return any(filename == f.stem for f in Path(dir_path).iterdir() if f.is_file())

def filename_in_dir(filename:str, dir_path:str):
    for f in Path(dir_path).iterdir():
        if filename in f.stem and f.is_file():
            return f

def extract_file(destination):
    # Extract the downloaded file
    print("Extracting file...")
    shutil.unpack_archive(destination, extract_dir=os.path.dirname(destination))
    print("Extraction completed.")


def open_file(filepath: str, opener = None):
    if opener is not None:
        subprocess.call([opener, os.path.abspath(filepath)])
    else:
        os.startfile(os.path.abspath(filepath))

def YesOrNo():
    while True:
        reply = str(input('[Y/N]: ')).lower().strip()
        if reply[:1] == 'y':
            return True
        if reply[:1] == 'n':
            return False 
        
def CheckMarkMsg(msg: str):
    print(f"{Fore.GREEN}\N{CHECK MARK}{Style.RESET_ALL} {msg}")