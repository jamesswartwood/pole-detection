from uwimg import *
import glob
import time

data_path = "data/samples/"
save_path = "output/samples/images/"
save_prefix = ""

filenames = glob.glob(data_path + "*.jpg")
filenames += glob.glob(data_path + "*.JPG")

print("Number of files:", len(filenames))

count = 0
start = time.time()

for name in filenames:
    image = load_image(name)
    res = detect_pole(image)
    save_image(res, save_path + save_prefix + name[len(data_path):-4])
    free_image(image)
    count += 1
    print(" ", round(count / len(filenames) * 100, 2), "% complete", sep="")

end = time.time()

print("Number of files processed:", len(filenames))
print("Runtime in seconds:", end - start)
if (len(filenames) != 0):
    print("Time in seconds per file:", (end - start) / len(filenames))