import sys
import os
import subprocess
import re
import time
from dataclasses import dataclass
from typing import List
import pandas

time_reg = re.compile("Checkpoint \d: ([\d\\.]{1,})")

def run_cmd(cmd):
    print(f"Running {cmd}")
    proc = subprocess.run(cmd, shell=True, capture_output=True)
    stdout = proc.stdout.decode()
    stderr = proc.stderr.decode()
    return stdout, stderr

@dataclass
class Result:
    program: str
    checkpoints: List[float]
    threads: int
    filesize: float

    @property
    def encoding_time(self):
        return self.checkpoints[2]

    @property
    def decoding_time(self):
        return self.checkpoints[4]

    def asdict(self):
        d = self.__dict__
        d['encoding_time'] = self.encoding_time
        d['decoding_time'] = self.decoding_time
        del d['checkpoints']
        return d

if __name__ == "__main__":
    in_dir = "../../inputs"
    inputs = sorted(os.listdir(in_dir))
    program = ["mpi.sh", "baseline", "baseline-8ecc", "omp", "omp-8ecc"]
    results = []
    for p in program:
        for i in inputs:
            if "7.txt" in i and "mpi" in p:
                continue
            for threads in range(1,17):
                if "baseline" in p and threads > 1:
                    break
                if p == "omp":
                    os.environ['OMP_NUM_THREADS'] = str(threads)
                infile = os.path.join(in_dir,i)
                filesize = os.stat(infile).st_size / 1000000
                count = f" {threads}" if "mpi" in p else ""
                stdout, stderr = run_cmd(f"./{p} {infile}{count}")
                checkpoint_times = [float(x) for x in time_reg.findall(stdout)]
                results.append(Result(p, checkpoint_times, threads, filesize))
            if "mpi" in p:
                for threads in [32,48,64,96]:
                    infile = os.path.join(in_dir,i)
                    filesize = os.stat(infile).st_size / 1000000
                    count = f" {threads}" if "mpi" in p else ""
                    stdout, stderr = run_cmd(f"./{p} {infile}{count}")
                    checkpoint_times = [float(x) for x in time_reg.findall(stdout)]
                    results.append(Result(p, checkpoint_times, threads, filesize))



    df = pandas.DataFrame([x.asdict() for x in results])

    df.to_csv("results.csv")
    print(df)
