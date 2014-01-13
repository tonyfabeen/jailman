#encoding: utf-8
module Jailman
  class Jail
    attr_accessor :directory, :name

    def initialize(name=nil)
      @directory = Dir.pwd
      @name      = name || @directory.split('/').last
    end

  end
end
