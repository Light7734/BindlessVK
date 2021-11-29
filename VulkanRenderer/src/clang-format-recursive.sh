find . -regex '.*\.\(cpp\|hpp\|cc\|cxx\|h\)' -exec clang-format -style=file -i --verbose {} \;
read -p "Press any key to continue"