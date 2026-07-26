@echo off
:loop
git add .
git commit -m "加了亿点点东西"
git push
goto loop