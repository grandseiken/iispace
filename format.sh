OIFS="$IFS"
IFS=$'\n'
for file in $(git ls-files | grep "\.cc$\|\.cpp\|\.h$\|\.proto$\|\.glsl$"); do
  clang-format -i $file
done