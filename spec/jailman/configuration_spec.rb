#encoding: utf-8

require 'spec_helper'
require 'jailman/configuration'

describe Jailman::Configuration do
  let(:jail) {Jailman::Jail.new('my_jail')}

  context 'when initialized' do
    context 'and no jail passed as argument' do

      it 'raises an Exception' do
        expect{ described_class.new }.to raise_error('A jail must be passed')
      end

    end

    context 'when a jail is filled' do

      it 'populates jail' do
        config  = described_class.new(jail)

        expect(config.jail).to_not be_nil
        expect(config.jail).to be(jail)
      end

    end

  end

  describe '#build' do

    it 'constructs a yaml file' do
      config  = described_class.new(jail)
      expect(config.build).to match(jail.name)
    end

  end

  describe '#create_yaml' do
    let(:config) do
      jail = Jailman::Jail.new("Jail#{rand(1000)}")
      jail.directory = Dir.pwd + "/spec/fixtures/jails/#{jail.name}"
      config = described_class.new(jail)
      config
    end

    before { FileUtils.mkdir_p(config.jail.directory) }

    it 'creates a new config file into current directory' do
      config.create_yaml
      yaml_path    = config.jail.directory + "/jail.yml"
      yaml_data = YAML.load_file(yaml_path)

      expect(File.exists?(yaml_path)).to be_true
      expect(yaml_data).to_not be_nil
      expect(yaml_data["application"]["name"]).to match(config.jail.name)
    end

    after { FileUtils.rmdir(config.jail.directory) }

  end

end
