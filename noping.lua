require 'oping'
require 'posix'

op = oping.new ()
op:host_add ("localhost")

while true do
  op:send ()

  iter = op:iterator_get ()
  while nil ~= iter do
    info = "from: " .. iter:get_info (oping.INFO_HOSTNAME) .. ", "
        .. "seq: " .. iter:get_info (oping.INFO_SEQUENCE) .. ", "
        .. "latency: " .. iter:get_info (oping.INFO_LATENCY)
    print (info)
    iter = iter:next ()
  end

  posix.sleep (1)
end
