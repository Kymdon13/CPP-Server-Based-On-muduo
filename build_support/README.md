### clang_format_exclusions.txt

if you wish to filter out certain patterns of files, you can add rules in the file (separate by `\n`):

```
*/util.*
*/Socket.*
```

because we use `fnmatch.fnmatch()` behind to do the matching, here is the matching rule for `fnmatch.fnmatch()`:

```python
import fnmatch

print(fnmatch.fnmatch('home/user/code/foo.txt', '*/*.txt'))  # 输出：True
print(fnmatch.fnmatch('home/user/code/foo.txt', '*/?oo.txt'))  # 输出：True
print(fnmatch.fnmatch('home/user/code/bar.txt', '*/*.txt'))  # 输出：True
print(fnmatch.fnmatch('home/user/code/Dat45.txt', '*/Dat[0-9]*.txt')) # 输出：True
print(fnmatch.fnmatch('home/user/code/Databc.txt', '*/Dat[0-9]*.txt')) # 输出：False
```