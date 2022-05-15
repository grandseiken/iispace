OIFS="$IFS"
IFS=$'\n'
for file in $(git ls-files | grep "\.cc$\|\.h$\|\.proto"); do
  clang-format -i $file
done