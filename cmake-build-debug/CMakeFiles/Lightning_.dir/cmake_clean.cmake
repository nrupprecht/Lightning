file(REMOVE_RECURSE
  "lib/libLightning_.a"
  "lib/libLightning_.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/Lightning_.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
