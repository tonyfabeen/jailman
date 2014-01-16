#encoding: utf-8
require 'celluloid/io'
require 'json'

module Jailman

  class Server

    include Celluloid::IO

    finalizer :finalize

    attr_reader :server, :socket_path

    def initialize(socket_path)
      puts "Starting Server..."
      @socket_path = socket_path
      @server = UNIXServer.open(socket_path)
      async.run
    end

    def run
      loop { async.handle_connection @server.accept }
    end

    def handle_connection(socket)
      loop do
        data = socket.readline
        puts "INCOMING DATA #{data}"

        #TODO: HERE will have the processing flow
        jail = JSON.parse(data)
        puts "Jail to create #{jail}"

        script = "../../bin/psc #{jail['name']} --create"
        p script
        output = `#{script}`
        raise output unless $? == 0
        script = "../../bin/ps_rootfs #{jail['name']} ../../spec/fixtures/jails/#{jail['name']}"
        p script
        output = `#{script}`
        raise output unless $? == 0
        script = "../../bin/psc #{jail['name']} --run free -m "
        p script
        output = `#{script}`
        p output
        raise output unless $? == 0


        #Response
        socket.write(data)
      end

      rescue EOFError
        puts "DISCONNECTED"

      ensure
        socket.close

    end

    def finalize
      puts "KILLED"

      if @server
        @server.close
        File.delete(@socket_path)
      end
    end

  end

end

supervisor = Jailman::Server.supervise('./test_sock.sock')
trap("INT"){ supervisor.terminate; exit }
loop {}

