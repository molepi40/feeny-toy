import os
import time

def test():
    start_time = time.time()
    os.system("bash ./run_tests.sh")
    end_time = time.time()

    total_time = end_time - start_time
    print(f"total test time: {total_time:04}s")

test()