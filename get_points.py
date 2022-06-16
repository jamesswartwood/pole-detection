from cmath import *
from uwimg import *
import glob
import time
import pandas as pd

data_path = "data/samples/"
save_path = "output/samples/"
save_prefix = "get_points_"

filenames = glob.glob(data_path + "*.jpg")
filenames += glob.glob(data_path + "*.JPG")

print("Number of files:", len(filenames))
print()

names = [None] * len(filenames)
top_x = [None] * len(filenames)
top_y = [None] * len(filenames)
bottom_x = [None] * len(filenames)
bottom_y = [None] * len(filenames)
pole_length = [None] * len(filenames)

count = 0
start = time.time()

for name in filenames:
    image = load_image(name)
    res = detect_pole_points(image)

    names[count] = name[len(data_path):]
    if res.top.x > 1: top_x[count] = round(res.top.x, 1)
    if res.top.y > 1: top_y[count] = round(res.top.y, 1)
    if res.bottom.x > 1: bottom_x[count] = round(res.bottom.x, 1)
    if res.bottom.y > 1: bottom_y[count] = round(res.bottom.y, 1)

    length = 0
    if res.top.x > 1 and res.top.y > 1 and res.bottom.x > 1 and res.bottom.y > 1:
        length = round(sqrt(pow(res.top.x - res.bottom.x, 2) +
                            pow(res.top.y - res.bottom.y, 2)).real, 5)
    if length > 1: pole_length[count] = length

    free_image(image)
    count += 1
    print(" ", round(count / len(filenames) * 100, 2), "% complete", sep="")

end = time.time()

print()
print("Runtime in seconds:", round(end - start, 4))
if (len(filenames) != 0):
    print("Avg. time in seconds per file:", round((end - start) / len(filenames), 4))

print()
d = {'filename': names,
        'top_x': top_x,
        'top_y': top_y,
        'bottom_x': bottom_x,
        'bottom_y': bottom_y,
        'pole_length': pole_length}
df = pd.DataFrame(data=d)
df.to_csv(save_path + save_prefix + "results.csv", index=False)
print("Saved", save_path + save_prefix + "results.csv")