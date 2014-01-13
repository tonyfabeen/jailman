#encoding: utf-8
module Jailman
  class Jail
    attr_accessor :directory

    def initialize
      @directory = Dir.pwd
    end

  end
end
