import hashlib
import json
import os

class HashChecker:
    def __init__(self, keyfile: str):
        '''
        :param keyfile: file that contains json for previous hashes
        '''
        self.keyfile_path:str = keyfile
        self.__key_data: dict | None = None

    def load_keyfile(self) -> dict:
        if os.path.exists(self.keyfile_path):
            with open(self.keyfile_path, "r") as f:
                return json.load(f)
        else:
            with open(self.keyfile_path, "x") as _:
                pass
            return {}

    def save_keyfile(self,):
        with open(self.keyfile_path, "w") as f:
            json.dump(self.__key_data, f)

    def __enter__(self):
        self.__key_data = self.load_keyfile()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.save_keyfile()
        self.__key_data = None

    def __compute_hash_of_file(self, file: str, hash_obj):
        with open(file, 'rb') as f:
            for chunk in iter(lambda: f.read(8192), b''):
                hash_obj.update(chunk)

    def compute_hash(self, keyname: str, files: list[str]) -> bool:
        if self.__key_data is None:
            raise Exception("You must 'with' Hashchecker")
        '''
        given a list of files and keyname, will check the generate hash against an entry in the keyfile of keyname
        :param keyname: the name of the key you want to store a hash and check future hashes against
        :param files: the list of files you wish to generate a hash for
        :return: returns false if they don't match or key doesn't exist, true otherwise
        '''
        hash_obj = hashlib.new("sha256")
        for path in files:
            assert os.path.exists(path) # failure state if one of the files doesn't exist
            self.__compute_hash_of_file(path, hash_obj)

        hash_hex = hash_obj.hexdigest()
        key_data = self.__key_data.get(keyname, "")
        self.__key_data[keyname] = hash_hex
        if hash_hex != key_data:
            return False
        return True

