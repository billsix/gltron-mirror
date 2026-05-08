function script_print(...)
   local args = {...}
   io.write("[script] ")
   for i=1,select('#', ...) do
      io.write(tostring(args[i]))
   end
   io.write("\n")
end

