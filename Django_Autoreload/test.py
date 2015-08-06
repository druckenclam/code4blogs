import autoreload
import os

def main():
    print("start printing")
    print(os.getcwd())
    import time; time.sleep(100)

if __name__ == '__main__':
    autoreload.main(main)
