#encoding: utf-8
require 'celluloid/io'
require 'json'

module Jailman

  class Client

    include Celluloid::IO
    finalizer :finalize

    def initialize(socket_path)
      puts "Connecting to #{socket_path}"
      @socket_path = socket_path
      @socket = UNIXSocket.new(socket_path)
    end

    def send(msg)
      puts "Send to Server #{msg}"
      json = JSON.generate(msg)
      @socket.puts(json)

      #ANSWER
      data = @socket.readline.chomp
      puts "Server Answered #{data}"
    end

    def finalize
      @socket.close if @socket
    end

  end

end

client = Jailman::Client.new('./test_sock.sock')
client.send({:param_one => rand(1000), :param_two => rand(2000)})

