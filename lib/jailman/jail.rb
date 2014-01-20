#encoding: utf-8
module Jailman

  class Jail

    attr_accessor :directory, :name, :running
    attr_reader   :configuration, :provisioner

    def initialize(name=nil, directory=nil)
      @directory = directory || Dir.pwd
      @name      = name || @directory.split('/').last
      @running   = false
      @provisioner = Jailman::Provisioner.new(self)
    end

    def create
      provisioner = Jailman::Provisioner.new(self)
      provisioner.run!

      configuration = Jailman::Configuration.new(self)
      configuration.create_yaml

      @running = true
    end

    def run(command)
      command = "#{Jailman::Constants::JAIL_SCRIPT} #{name} --run #{command}"
      Jailman::CommandRunner.run(command)
    end

    def status
      running ? "STARTED" : "STOPPED"
    end


    def stop
      @provisioner.stop!
      @running = false
    end



  end

end
