#encoding: utf-8
module Jailman

  class Jail

    attr_accessor :directory, :name, :pid
    attr_reader   :configuration, :provisioner

    def initialize(name=nil, directory=nil)
      @directory = directory || Jailman::Constants::ROOTFS_DIR
      @name      = name || @directory.split('/').last
      @provisioner = Jailman::Provisioner.new(self)
    end

    def create
      provisioner = Jailman::Provisioner.new(self)
      provisioner.run!

      configuration = Jailman::Configuration.new(self)
      configuration.create_yaml
    end

    def run(command)
      command = "#{Jailman::Constants::JAIL_SCRIPT} #{name} --run #{command}"
      Jailman::CommandRunner.run(command)
    end

    def status
     running? ? "STARTED" : "DESTROYED"
    end

    def running?
      File.exists?("#{Jailman::Constants::PID_DIR}/#{name}.pid")
    end

    def destroy
      @provisioner.destroy!
    end

    def self.find(name)
      state = Jailman::State.new(self.new(name))
      state.load
    end

    def self.list
      jails = Dir.entries(Jailman::Constants::STATE_DIR).map do |item|
        Jailman::Jail.find(item) unless item.include?(".")
      end

      rows = []
      rows << ["PID", "NAME", "DIRECTORY", "STATUS"]

      jails.each do |jail|
        rows << [jail.pid, jail.name, jail.directory, jail.status] if jail
      end

      table = Terminal::Table.new :rows => rows
      table
    end

  end

end
