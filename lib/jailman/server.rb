#encoding: utf-8
require 'celluloid/io'

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

