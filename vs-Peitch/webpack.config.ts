import * as path from 'path';
import * as webpack from 'webpack';

const extensionConfig: webpack.Configuration = {
  target: 'node',
  mode: 'none', 

  entry: './src/extension.ts',
  output: {
    path: path.resolve(__dirname, 'dist'),
    filename: 'extension.js',
    libraryTarget: 'commonjs2',
  },
  externals: {
    vscode: 'commonjs vscode', 
  },
  resolve: {
    extensions: ['.ts', '.js'],
  },
  module: {
    rules: [
      {
        test: /\.ts$/,
        exclude: /node_modules/,
        use: [
          {
            loader: 'ts-loader',
          },
        ],
      },
    ],
  },
  devtool: 'nosources-source-map',
  infrastructureLogging: {
    level: "log",
  },
};

const webviewConfig: webpack.Configuration = {
    target: 'web',
    mode: 'none',
    entry: './src/ui/main.tsx',
    output: {
        path: path.resolve(__dirname, 'dist'),
        filename: 'webview.js',
    },
    resolve: {
        extensions: ['.ts', '.tsx', '.js', '.jsx', '.css'],
    },
    module: {
        rules: [
            {
                test: /\.tsx?$/,
                exclude: /node_modules/,
                use: 'ts-loader',
            },
            {
                test: /\.css$/,
                use: [
                    'style-loader',
                    'css-loader',
                    'postcss-loader',
                ],
            },
        ],
    },
    devtool: 'nosources-source-map',
};

export default [extensionConfig, webviewConfig];
