curl -o %DepsBasePath%.zip -kLO https://cdn-fastly.obsproject.com/downloads/dependencies2019.zip -f --retry 5 -C -
7z x %DepsBasePath%.zip -o%DepsBasePath% -aoa
